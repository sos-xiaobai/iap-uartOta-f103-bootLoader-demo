/**
  ******************************************************************************
  * @file           : bootloader_uart.h
  * @brief          : UART communication protocol header for IAP
  ******************************************************************************
  * @attention
  *
  * This is an example implementation of UART-based IAP protocol.
  *
  ******************************************************************************
  */

#ifndef __BOOTLOADER_UART_H
#define __BOOTLOADER_UART_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include "main.h"

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  IAP数据包结构体（用于UART通信）
  */
typedef struct
{
    uint8_t  cmd;                 /* 命令字节 */
    uint16_t len;                 /* 数据长度 */
    uint32_t addr;                /* 目标地址 */
    uint8_t  data[1024];          /* 数据缓冲区 */
} IAP_Packet_TypeDef;

/* Exported constants --------------------------------------------------------*/

/* IAP命令定义 */
#define IAP_CMD_ERASE               0xA0    /* 擦除Flash */
#define IAP_CMD_WRITE               0xA1    /* 写入数据 */
#define IAP_CMD_READ                0xA2    /* 读取数据 */
#define IAP_CMD_JUMP                0xA3    /* 跳转到APP */
#define IAP_CMD_GET_INFO            0xA4    /* 获取Bootloader信息 */
#define IAP_CMD_START_UPDATE        0xA5    /* 开始升级 */
#define IAP_CMD_END_UPDATE          0xA6    /* 结束升级 */
#define IAP_CMD_VERIFY              0xA7    /* 校验固件 */
#define IAP_CMD_GET_APP_VERSION     0xA8    /* 获取APP固件版本信息 */
#define IAP_CMD_SET_APP_VERSION     0xA9    /* 设置APP固件版本信息 */
#define IAP_CMD_COMPARE_VERSION     0xAA    /* 比较版本（判断是否需要升级） */

/* Exported macro ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  初始化UART IAP协议
  * @param  huart: UART句柄指针
  * @retval None
  * 
  * @note   在使用UART协议前必须先调用此函数
  *         例如: Bootloader_UART_Init(&huart1);
  */
void Bootloader_UART_Init(UART_HandleTypeDef *huart);

/**
  * @brief  UART IAP协议处理
  * @retval None
  * 
  * @note   此函数应在主循环中调用，用于处理接收到的IAP命令
  *         例如: 
  *         while(1) {
  *             Bootloader_UART_Process();
  *         }
  */
void Bootloader_UART_Process(void);

/**
  * @brief  通过UART发送字符串（用于调试）
  * @param  str: 字符串指针
  * @retval None
  * 
  * @note   此函数主要用于发送调试信息
  *         例如: Bootloader_UART_SendString("Bootloader Started\r\n");
  */
void Bootloader_UART_SendString(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* __BOOTLOADER_UART_H */

