/**
  ******************************************************************************
  * @file           : bootloader_uart.c
  * @brief          : UART communication protocol for IAP
  ******************************************************************************
  * @attention
  *
  * This is an example implementation of UART-based IAP protocol.
  * You need to initialize UART before using these functions.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "bootloader_uart.h"
#include "bootloader.h"
#include <string.h>

extern UART_HandleTypeDef huart1;

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

#define UART_TIMEOUT_MS         5000    /* UART超时时间 */

/* 数据包帧格式 */
#define PACKET_HEADER_SIZE      13      /* 包头大小: Header(2) + Cmd(1) + Len(2) + Addr(4) + CRC(4) */
#define PACKET_HEADER           0xAA55  /* 数据包头部标识 */
#define PACKET_TAIL             0x55AA  /* 数据包尾部标识 */

/* 响应码 */
#define RESPONSE_SUCCESS        0x00
#define RESPONSE_ERROR          0x01
#define RESPONSE_CRC_ERROR      0x02
#define RESPONSE_FLASH_ERROR    0x03
#define RESPONSE_INVALID_CMD    0x04
#define RESPONSE_INVALID_PARAM  0x05

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static UART_HandleTypeDef *p_huart = NULL;

/* 使用静态变量避免栈溢出（结构体大小约1034字节） */
static IAP_Packet_TypeDef rx_packet;      /* 接收数据包缓冲区 */
static IAP_Packet_TypeDef tx_packet;      /* 发送数据包缓冲区 */

/* Private function prototypes -----------------------------------------------*/
static uint8_t ReceivePacket(IAP_Packet_TypeDef *packet);
static void SendResponse(uint8_t cmd, uint8_t status, uint8_t *data, uint16_t len);
static uint32_t CalculatePacketCRC(IAP_Packet_TypeDef *packet);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  初始化UART IAP协议
  * @param  huart: UART句柄指针
  * @retval None
  */
void Bootloader_UART_Init(UART_HandleTypeDef *huart)
{
    p_huart = huart;
}

/**
  * @brief  UART IAP协议处理（主循环中调用）
  * @retval None
  */
void Bootloader_UART_Process(void)
{
    IAP_StatusTypeDef status;
    uint8_t response_code = RESPONSE_SUCCESS;
    Bootloader_InfoTypeDef info;
    
    /* 安全检查1：UART句柄 */
    if (p_huart == NULL)
    {
        return;
    }
    
    /* 安全检查2：UART状态 */
    if (p_huart->Instance == NULL)
    {
        return;
    }
    
    /* 安全检查3：UART是否初始化 */
    if (p_huart->gState == HAL_UART_STATE_RESET)
    {
        return;
    }
    
    /* 接收数据包（使用静态缓冲区避免栈溢出） */
    if (ReceivePacket(&rx_packet) != 0)
    {
        return;  /* 没有收到完整数据包 */
    }
    
    /* 处理命令 */
    switch (rx_packet.cmd)
    {
        case IAP_CMD_GET_INFO:
            /* 获取Bootloader信息 */
            Bootloader_GetInfo(&info);
            SendResponse(IAP_CMD_GET_INFO, RESPONSE_SUCCESS, 
                        (uint8_t*)&info, sizeof(Bootloader_InfoTypeDef));
            break;
            
        case IAP_CMD_START_UPDATE:
            /* 开始升级 */
            status = IAP_StartUpdate();
            response_code = (status == IAP_SUCCESS) ? RESPONSE_SUCCESS : RESPONSE_ERROR;
            SendResponse(IAP_CMD_START_UPDATE, response_code, NULL, 0);
            break;
            
        case IAP_CMD_WRITE:
            /* 写入数据 */
            status = IAP_WriteData(rx_packet.addr, rx_packet.data, rx_packet.len);
            if (status == IAP_SUCCESS)
            {
                response_code = RESPONSE_SUCCESS;
            }
            else if (status == IAP_FLASH_ERROR)
            {
                response_code = RESPONSE_FLASH_ERROR;
            }
            else
            {
                response_code = RESPONSE_ERROR;
            }
            SendResponse(IAP_CMD_WRITE, response_code, NULL, 0);
            break;
            
        case IAP_CMD_READ:
            /* 读取数据 */
            if (rx_packet.len <= 1024)
            {
                status = Bootloader_Flash_Read(rx_packet.addr, rx_packet.data, rx_packet.len);
                if (status == IAP_SUCCESS)
                {
                    SendResponse(IAP_CMD_READ, RESPONSE_SUCCESS, rx_packet.data, rx_packet.len);
                }
                else
                {
                    SendResponse(IAP_CMD_READ, RESPONSE_ERROR, NULL, 0);
                }
            }
            else
            {
                SendResponse(IAP_CMD_READ, RESPONSE_INVALID_PARAM, NULL, 0);
            }
            break;
            
        case IAP_CMD_VERIFY:
            /* 校验固件 */
            {
                uint32_t expected_crc = *((uint32_t*)rx_packet.data);
                status = IAP_VerifyApp(expected_crc);
                response_code = (status == IAP_SUCCESS) ? RESPONSE_SUCCESS : RESPONSE_CRC_ERROR;
                SendResponse(IAP_CMD_VERIFY, response_code, NULL, 0);
            }
            break;
            
        case IAP_CMD_END_UPDATE:
            /* 结束升级 */
            status = IAP_EndUpdate();
            response_code = (status == IAP_SUCCESS) ? RESPONSE_SUCCESS : RESPONSE_ERROR;
            SendResponse(IAP_CMD_END_UPDATE, response_code, NULL, 0);
            
            /* 升级成功后延迟复位，让响应数据发送完成 */
            if (status == IAP_SUCCESS)
            {
                HAL_Delay(200);  /* 等待响应发送完成 */
                NVIC_SystemReset();  /* 系统复位，重新启动 */
            }
            break;
            
        case IAP_CMD_JUMP:
            /* 跳转到APP（通过系统复位方式更可靠） */
            SendResponse(IAP_CMD_JUMP, RESPONSE_SUCCESS, NULL, 0);
            HAL_Delay(200);  /* 等待响应发送完成 */
            NVIC_SystemReset();  /* 系统复位，让Bootloader重新启动并跳转到APP */
            break;
            
        case IAP_CMD_ERASE:
            /* 擦除Flash */
            status = Bootloader_Flash_Erase(rx_packet.addr, rx_packet.len);
            response_code = (status == IAP_SUCCESS) ? RESPONSE_SUCCESS : RESPONSE_FLASH_ERROR;
            SendResponse(IAP_CMD_ERASE, response_code, NULL, 0);
            break;
            
        case IAP_CMD_GET_APP_VERSION:
            /* 获取APP固件版本信息 */
            {
                Firmware_VersionTypeDef app_version;
                status = Bootloader_ReadAppVersion(&app_version);
                if (status == IAP_SUCCESS)
                {
                    SendResponse(IAP_CMD_GET_APP_VERSION, RESPONSE_SUCCESS, 
                               (uint8_t*)&app_version, sizeof(Firmware_VersionTypeDef));
                }
                else
                {
                    SendResponse(IAP_CMD_GET_APP_VERSION, RESPONSE_ERROR, NULL, 0);
                }
            }
            break;
            
        case IAP_CMD_SET_APP_VERSION:
            /* 设置APP固件版本信息 */
            {
                Firmware_VersionTypeDef *new_version = (Firmware_VersionTypeDef*)rx_packet.data;
                status = Bootloader_SaveAppVersion(new_version);
                response_code = (status == IAP_SUCCESS) ? RESPONSE_SUCCESS : RESPONSE_ERROR;
                SendResponse(IAP_CMD_SET_APP_VERSION, response_code, NULL, 0);
            }
            break;
            
        case IAP_CMD_COMPARE_VERSION:
            /* 比较版本号，判断是否需要升级 */
            {
                Firmware_VersionTypeDef current_version;
                Firmware_VersionTypeDef *new_version = (Firmware_VersionTypeDef*)rx_packet.data;
                int8_t result;
                uint8_t response_data[2];
                
                /* 读取当前版本 */
                status = Bootloader_ReadAppVersion(&current_version);
                if (status != IAP_SUCCESS)
                {
                    /* 当前没有有效版本信息，建议升级 */
                    response_data[0] = 1;  /* 需要升级 */
                    response_data[1] = 0;  /* 无当前版本 */
                    SendResponse(IAP_CMD_COMPARE_VERSION, RESPONSE_SUCCESS, response_data, 2);
                }
                else
                {
                    /* 比较版本 */
                    result = Bootloader_CompareVersion(new_version, &current_version);
                    response_data[0] = (result > 0) ? 1 : 0;  /* 是否需要升级 */
                    response_data[1] = result;  /* 比较结果 */
                    SendResponse(IAP_CMD_COMPARE_VERSION, RESPONSE_SUCCESS, response_data, 2);
                }
            }
            break;
            
        default:
            /* 未知命令 */
            SendResponse(rx_packet.cmd, RESPONSE_INVALID_CMD, NULL, 0);
            break;
    }
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  接收数据包
  * @param  packet: 数据包结构体指针
  * @retval 0: 成功, 1: 失败或未收到完整数据包
  */
static uint8_t ReceivePacket(IAP_Packet_TypeDef *packet)
{
    HAL_StatusTypeDef status;
    uint16_t header;
    uint32_t received_crc, calculated_crc;
    
    /* 安全检查：确保UART句柄有效 */
    if (p_huart == NULL)
    {
        return 1;  /* UART未初始化 */
    }
    
    /* 接收包头 (2字节) */
    /* 超时时间500ms：既不会阻塞太久，又有足够时间接收数据 */
    status = HAL_UART_Receive(&huart1, (uint8_t*)&header, 2, HAL_MAX_DELAY);
    if (status != HAL_OK || header != PACKET_HEADER)
    {
        return 1;  /* 超时或包头错误，直接返回 */
    }
    
    /* 接收命令字节 (1字节) */
    status = HAL_UART_Receive(p_huart, &packet->cmd, 1, UART_TIMEOUT_MS);
    if (status != HAL_OK)
    {
        return 1;
    }
    
    /* 接收数据长度 (2字节) */
    status = HAL_UART_Receive(p_huart, (uint8_t*)&packet->len, 2, UART_TIMEOUT_MS);
    if (status != HAL_OK)
    {
        return 1;
    }
    
    /* 接收地址 (4字节) */
    status = HAL_UART_Receive(p_huart, (uint8_t*)&packet->addr, 4, UART_TIMEOUT_MS);
    if (status != HAL_OK)
    {
        return 1;
    }
    
    /* 接收数据 */
    if (packet->len > 0)
    {
        if (packet->len > sizeof(packet->data))
        {
            return 1;  /* 数据长度超出缓冲区 */
        }
        
        status = HAL_UART_Receive(p_huart, packet->data, packet->len, UART_TIMEOUT_MS);
        if (status != HAL_OK)
        {
            return 1;
        }
    }
    
    /* 接收CRC (4字节) */
    status = HAL_UART_Receive(p_huart, (uint8_t*)&received_crc, 4, UART_TIMEOUT_MS);
    if (status != HAL_OK)
    {
        return 1;
    }
    
    /* 验证CRC */
    calculated_crc = CalculatePacketCRC(packet);
    if (received_crc != calculated_crc)
    {
        SendResponse(packet->cmd, RESPONSE_CRC_ERROR, NULL, 0);
        return 1;
    }
    
    return 0;  /* 成功 */
}

/**
  * @brief  发送响应
  * @param  cmd: 命令字节
  * @param  status: 状态码
  * @param  data: 数据指针（可为NULL）
  * @param  len: 数据长度
  * @retval None
  */
static void SendResponse(uint8_t cmd, uint8_t status, uint8_t *data, uint16_t len)
{
    uint16_t header = PACKET_HEADER;
    uint32_t crc;
    
    if (p_huart == NULL)
    {
        return;
    }
    
    /* 构建响应包（使用静态缓冲区避免栈溢出） */
    tx_packet.cmd = cmd;
    tx_packet.len = len + 1;  /* 状态码也算在长度内 */
    tx_packet.addr = 0;
    tx_packet.data[0] = status;
    
    if (data != NULL && len > 0)
    {
        memcpy(&tx_packet.data[1], data, len);
    }
    
    /* 计算CRC */
    crc = CalculatePacketCRC(&tx_packet);
    
    /* 发送包头 */
    HAL_UART_Transmit(p_huart, (uint8_t*)&header, 2, UART_TIMEOUT_MS);
    
    /* 发送命令 */
    HAL_UART_Transmit(p_huart, &tx_packet.cmd, 1, UART_TIMEOUT_MS);
    
    /* 发送长度 */
    HAL_UART_Transmit(p_huart, (uint8_t*)&tx_packet.len, 2, UART_TIMEOUT_MS);
    
    /* 发送地址 */
    HAL_UART_Transmit(p_huart, (uint8_t*)&tx_packet.addr, 4, UART_TIMEOUT_MS);
    
    /* 发送数据 */
    if (tx_packet.len > 0)
    {
        HAL_UART_Transmit(p_huart, tx_packet.data, tx_packet.len, UART_TIMEOUT_MS);
    }
    
    /* 发送CRC */
    HAL_UART_Transmit(p_huart, (uint8_t*)&crc, 4, UART_TIMEOUT_MS);
}

/**
  * @brief  计算数据包的CRC32
  * @param  packet: 数据包指针
  * @retval CRC32值
  */
static uint32_t CalculatePacketCRC(IAP_Packet_TypeDef *packet)
{
    uint8_t temp_buffer[1200];
    uint16_t index = 0;
    
    /* 组合需要计算CRC的数据 */
    temp_buffer[index++] = packet->cmd;
    temp_buffer[index++] = (uint8_t)(packet->len & 0xFF);
    temp_buffer[index++] = (uint8_t)((packet->len >> 8) & 0xFF);
    temp_buffer[index++] = (uint8_t)(packet->addr & 0xFF);
    temp_buffer[index++] = (uint8_t)((packet->addr >> 8) & 0xFF);
    temp_buffer[index++] = (uint8_t)((packet->addr >> 16) & 0xFF);
    temp_buffer[index++] = (uint8_t)((packet->addr >> 24) & 0xFF);
    
    if (packet->len > 0)
    {
        memcpy(&temp_buffer[index], packet->data, packet->len);
        index += packet->len;
    }
    
    /* 计算CRC32 */
    return Bootloader_CalculateCRC32(temp_buffer, index);
}

/**
  * @brief  简单的发送字符串函数（用于调试）
  * @param  str: 字符串
  * @retval None
  */
void Bootloader_UART_SendString(const char *str)
{
    if (p_huart != NULL && str != NULL)
    {
        HAL_UART_Transmit(p_huart, (uint8_t*)str, strlen(str), UART_TIMEOUT_MS);
    }
}

