/**
  ******************************************************************************
  * @file           : bootloader.h
  * @brief          : Bootloader header file for IAP (In-Application Programming)
  ******************************************************************************
  * @attention
  *
  * This file contains all the IAP bootloader related definitions, structures,
  * and function prototypes.
  *
  ******************************************************************************
  */

#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdio.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  IAP升级状态枚举
  */
typedef enum
{
    IAP_SUCCESS = 0,              /* 操作成功 */
    IAP_ERROR,                    /* 通用错误 */
    IAP_FLASH_ERROR,              /* Flash操作错误 */
    IAP_VERIFY_ERROR,             /* 校验错误 */
    IAP_TIMEOUT,                  /* 超时错误 */
    IAP_INVALID_PARAM,            /* 参数无效 */
    IAP_APP_INVALID,              /* APP无效 */
} IAP_StatusTypeDef;

/**
  * @brief  IAP命令定义
  * @note   这些命令用于IAP通信协议
  *         如果使用UART协议，这些定义在 bootloader_uart.h 中
  */
#ifndef IAP_CMD_ERASE
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
#endif /* IAP_CMD_ERASE */

/**
  * @brief  IAP数据包结构体
  */
typedef struct
{
    uint8_t  cmd;                 /* 命令字节 */
    uint16_t len;                 /* 数据长度 */
    uint32_t addr;                /* 目标地址 */
    uint8_t  data[1024];          /* 数据缓冲区 */
    uint32_t crc;                 /* CRC校验值 */
} IAP_PacketTypeDef;

/**
  * @brief  Bootloader信息结构体
  */
typedef struct
{
    uint8_t  version[3];          /* 版本号: [主版本号, 次版本号, 修订号] */
    uint32_t bootloader_size;     /* Bootloader大小 */
    uint32_t app_start_addr;      /* APP起始地址 */
    uint32_t app_max_size;        /* APP最大大小 */
    uint8_t  mcu_type[16];        /* MCU型号 */
} Bootloader_InfoTypeDef;

/**
  * @brief  固件版本信息结构体
  * @note   此结构体用于保存APP固件的版本信息
  *         存储在Flash的固定位置，用于版本管理和升级决策
  */
typedef struct
{
    uint32_t magic;               /* 魔术字: 0x46575652 ("FWVR") 用于验证数据有效性 */
    uint8_t  major;               /* 主版本号 */
    uint8_t  minor;               /* 次版本号 */
    uint16_t patch;               /* 修订号 */
    uint32_t build_date;          /* 编译日期: YYYYMMDD格式，例如：20241106 */
    uint32_t build_time;          /* 编译时间: HHMMSS格式，例如：153045 */
    uint32_t app_size;            /* APP固件实际大小（字节） */
    uint32_t app_crc32;           /* APP固件CRC32校验值 */
    char     description[32];     /* 固件描述信息 */
    uint32_t reserved[4];         /* 保留字段，用于未来扩展 */  //reserved[0]存储app运行状态 
} Firmware_VersionTypeDef;

/* Exported constants --------------------------------------------------------*/

/* Flash地址配置 */
#define BOOTLOADER_START_ADDR   0x08000000  /* Bootloader起始地址 */
#define BOOTLOADER_SIZE         0x00004000  /* Bootloader大小: 16KB */
#define APP_START_ADDR          0x08004000  /* APP起始地址: 16KB后 */
#define APP_MAX_SIZE            0x0000BC00  /* APP 最大大小: 47KB（留出最后一页给标志） */
#define FLASH_END_ADDR          0x08010000  /* Flash结束地址:64KB */

/* 备份区配置（可选，用于断电恢复） */
#define BACKUP_START_ADDR       0x08040000  /* 备份区起始地址 */
#define BACKUP_SIZE             0x00040000  /* 备份区大小: 256KB */

/* APP有效性检查 */
#define APP_STACK_VALID_MIN     0x20001000  /* RAM起始地址 */
#define APP_STACK_VALID_MAX     0x20005000  /* RAM结束地址: 20KB */

/* Flash操作配置 */
/* 注意: FLASH_PAGE_SIZE 由 STM32 HAL库定义 (stm32f1xx_hal_flash_ex.h)
 * STM32F103ZE 大容量设备: 0x800 (2KB)
 * 如果需要手动定义，请取消下面的注释：
 */
// #ifndef FLASH_PAGE_SIZE
// #define FLASH_PAGE_SIZE         0x00000800  /* Flash页大小: 2KB */
// #endif

/* 超时配置 */
#define IAP_TIMEOUT_MS          5000        /* IAP操作超时时间: 5秒 */
#define JUMP_DELAY_MS           100         /* 跳转前延迟时间 */

/* 升级标志地址（使用Flash最后一页存储标志和版本信息） */
#define UPDATE_FLAG_ADDR        0x0800FC00  /* 升级标志地址 */
#define UPDATE_FLAG_VALUE       0xAA55AA55  /* 升级标志值 */

/* 固件版本信息地址（存储在Flash最后一页，UPDATE_FLAG之后） */
#define FW_VERSION_INFO_ADDR    0x0800FD00    /* 固件版本信息地址 */
#define FW_VERSION_MAGIC        0x46575652  /* 固件版本魔术字 "FWVR" */

#define APP_STATUS_ADDR 0x0800FC04    //app运行保存的数据 


/* Bootloader版本信息 */
#define BOOTLOADER_VERSION_MAJOR    1
#define BOOTLOADER_VERSION_MINOR    0
#define BOOTLOADER_VERSION_REVISION 0

/* 通信协议配置 */
#define IAP_PACKET_HEADER       0xAA55      /* 数据包头部标识 */
#define IAP_PACKET_TAIL         0x55AA      /* 数据包尾部标识 */

/* Exported macro ------------------------------------------------------------*/

/**
  * @brief  计算Flash页地址
  */
#define FLASH_PAGE_ADDR(page)   (BOOTLOADER_START_ADDR + (page) * FLASH_PAGE_SIZE)

/**
  * @brief  判断地址是否在APP区域内
  */
#define IS_APP_ADDRESS(addr)    (((addr) >= APP_START_ADDR) && ((addr) < (APP_START_ADDR + APP_MAX_SIZE)))

/**
  * @brief  判断地址是否在有效Flash范围内
  */
#define IS_VALID_FLASH_ADDR(addr) (((addr) >= BOOTLOADER_START_ADDR) && ((addr) < FLASH_END_ADDR))

/* Exported functions prototypes ---------------------------------------------*/

/* Bootloader初始化和信息 */
void Bootloader_Init(void);
void Bootloader_GetInfo(Bootloader_InfoTypeDef *info);

/* APP检查和跳转 */
uint8_t Bootloader_CheckAppValid(void);
void Bootloader_JumpToApp(void);

/* Flash操作函数 */
IAP_StatusTypeDef Bootloader_Flash_Erase(uint32_t start_addr, uint32_t size);
IAP_StatusTypeDef Bootloader_Flash_Write(uint32_t addr, uint8_t *data, uint32_t len);
IAP_StatusTypeDef Bootloader_Flash_Read(uint32_t addr, uint8_t *data, uint32_t len);

/* IAP升级流程 */
IAP_StatusTypeDef IAP_StartUpdate(void);
IAP_StatusTypeDef IAP_WriteData(uint32_t addr, uint8_t *data, uint32_t len);
IAP_StatusTypeDef IAP_EndUpdate(void);
IAP_StatusTypeDef IAP_VerifyApp(uint32_t expected_crc);

/* 升级标志管理 */
void Bootloader_SetUpdateFlag(void);
void Bootloader_ClearUpdateFlag(void);
uint8_t Bootloader_CheckUpdateFlag(void);

/* CRC计算 */
uint32_t Bootloader_CalculateCRC32(uint8_t *data, uint32_t len);
uint32_t Bootloader_GetAppCRC32(void);

/* 工具函数 */
void Bootloader_DeInit(void);
void Bootloader_ResetDevice(void);

/* 调试和状态 */
const char* Bootloader_GetStatusString(IAP_StatusTypeDef status);

/* 固件版本管理 */
IAP_StatusTypeDef Bootloader_UpdateAppVersion(void);
IAP_StatusTypeDef Bootloader_SaveAppVersion(Firmware_VersionTypeDef *version);
IAP_StatusTypeDef Bootloader_ReadAppVersion(Firmware_VersionTypeDef *version);
IAP_StatusTypeDef Bootloader_SaveAppStatus(uint32_t status_word);
IAP_StatusTypeDef Bootloader_ReadAppStatus(uint32_t *status_word);
uint8_t Bootloader_IsAppVersionValid(void);
int8_t Bootloader_CompareVersion(Firmware_VersionTypeDef *ver1, Firmware_VersionTypeDef *ver2);
void Bootloader_EraseAppVersion(void);

#ifdef __cplusplus
}
#endif

#endif /* __BOOTLOADER_H */

