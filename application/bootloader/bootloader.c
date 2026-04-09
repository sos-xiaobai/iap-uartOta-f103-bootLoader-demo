/**
  ******************************************************************************
  * @file           : bootloader.c
  * @brief          : Bootloader implementation for IAP (In-Application Programming)
  ******************************************************************************
  * @attention
  *
  * This file contains all the IAP bootloader implementation including:
  * - Flash operations (erase, write, read)
  * - App validation and jump
  * - Firmware update process
  * - CRC calculation and verification
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "bootloader.h"
#include "protocol.h"
#include "main.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* CRC查找表（用于快速CRC32计算） */
static const uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static uint8_t update_in_progress = 0;  /* 升级进行中标志 */

/* Private function prototypes -----------------------------------------------*/
static uint32_t GetFlashPage(uint32_t addr);
static HAL_StatusTypeDef FlashWriteWord(uint32_t addr, uint32_t data);

/* Private user code ---------------------------------------------------------*/

/**
  * @brief  Bootloader初始化
  * @retval None
  */
void Bootloader_Init(void)
{
    /* 清除升级进行中标志 */
    update_in_progress = 0;
    
    /* 可以在这里添加其他初始化代码 */
    /* 例如：初始化LED、串口等 */
}

/**
  * @brief  获取Bootloader信息
  * @param  info: 信息结构体指针
  * @retval None
  */
void Bootloader_GetInfo(Bootloader_InfoTypeDef *info)
{
    if (info != NULL)
    {
        info->version[0] = BOOTLOADER_VERSION_MAJOR;
        info->version[1] = BOOTLOADER_VERSION_MINOR;
        info->version[2] = BOOTLOADER_VERSION_REVISION;
        info->bootloader_size = BOOTLOADER_SIZE;
        info->app_start_addr = APP_START_ADDR;
        info->app_max_size = APP_MAX_SIZE;
        snprintf((char*)info->mcu_type, sizeof(info->mcu_type), "STM32F103C8");
    }
}

/**
  * @brief  检查APP程序是否有效
  * @retval 1: APP有效, 0: APP无效
  */
uint8_t Bootloader_CheckAppValid(void)
{
    uint32_t stackTop = *(__IO uint32_t*)APP_START_ADDR;
    uint32_t resetVector = *(__IO uint32_t*)(APP_START_ADDR + 4);
    
    /* 检查栈顶地址是否在有效的RAM范围内 */
    if ((stackTop >= APP_STACK_VALID_MIN) && (stackTop < APP_STACK_VALID_MAX))
    {
        /* 检查复位向量是否在APP区域内（必须是奇数，Thumb指令集） */
        if ((resetVector >= APP_START_ADDR) && (resetVector < (APP_START_ADDR + APP_MAX_SIZE)))
        {
            /* 检查复位向量是否为Thumb指令（最低位应该为1） */
            if (resetVector & 0x00000001)
            {
                return 1;
            }
        }
    }
    return 0;
}

/**
  * @brief  跳转到APP程序
  * @retval None
  */
void Bootloader_JumpToApp(void)
{
    typedef void (*pFunction)(void);
    
    uint32_t jumpAddress;
    pFunction JumpToApplication;
    
    /* 检查APP是否有效 */
    if (Bootloader_CheckAppValid())
    {
        /* 关闭所有中断 */
        __disable_irq();
        
        /* 关闭SysTick */
        SysTick->CTRL = 0;
        SysTick->LOAD = 0;
        SysTick->VAL = 0;
        
        /* 禁用所有外设中断 */
        for (uint8_t i = 0; i < 8; i++)
        {
            NVIC->ICER[i] = 0xFFFFFFFF;
            NVIC->ICPR[i] = 0xFFFFFFFF;
        }
        
        /* 反初始化HAL库 */
        HAL_DeInit();
        
        /* 获取APP的复位向量地址 */
        jumpAddress = *(__IO uint32_t*)(APP_START_ADDR + 4);
        JumpToApplication = (pFunction)jumpAddress;
        
        /* 设置APP的栈顶地址 */
        __set_MSP(*(__IO uint32_t*)APP_START_ADDR);
        
        /* 重新设置中断向量表到APP的位置 */
        SCB->VTOR = APP_START_ADDR;
        
        /* 启用全局中断 */
        __enable_irq();
        
        /* 跳转到APP */
        JumpToApplication();
    }
}

/**
  * @brief  擦除Flash
  * @param  start_addr: 起始地址
  * @param  size: 要擦除的大小（字节）
  * @retval IAP状态
  */
IAP_StatusTypeDef Bootloader_Flash_Erase(uint32_t start_addr, uint32_t size)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;
    uint32_t start_page, end_page, num_pages;
    
    /* 参数检查 */
    if (!IS_VALID_FLASH_ADDR(start_addr) || size == 0)
    {
        return IAP_INVALID_PARAM;
    }
    
    /* 不能擦除Bootloader区域 */
    if (start_addr < APP_START_ADDR)
    {
        return IAP_INVALID_PARAM;
    }
    
    /* 计算起始页和结束页 */
    start_page = GetFlashPage(start_addr);
    end_page = GetFlashPage(start_addr + size - 1);
    num_pages = end_page - start_page + 1;
    
    /* 解锁Flash */
    status = HAL_FLASH_Unlock();
    if (status != HAL_OK)
    {
        return IAP_FLASH_ERROR;
    }
    
    /* 配置擦除参数 */
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = start_addr;
    EraseInitStruct.NbPages = num_pages;
    
    /* 执行擦除 */
    status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
    
    /* 锁定Flash */
    HAL_FLASH_Lock();
    
    if (status != HAL_OK)
    {
        return IAP_FLASH_ERROR;
    }
    
    return IAP_SUCCESS;
}

/**
  * @brief  写入Flash
  * @param  addr: 目标地址（必须4字节对齐）
  * @param  data: 数据缓冲区
  * @param  len: 数据长度（字节）
  * @retval IAP状态
  */
IAP_StatusTypeDef Bootloader_Flash_Write(uint32_t addr, uint8_t *data, uint32_t len)
{
    HAL_StatusTypeDef status;
    uint32_t i;
    uint32_t word_data;
    uint32_t write_addr = addr;
    uint32_t remaining = len;
    uint8_t *ptr = data;
    
    /* 参数检查 */
    if (!IS_APP_ADDRESS(addr) || data == NULL || len == 0)
    {
        return IAP_INVALID_PARAM;
    }
    
    /* 检查是否会超出APP区域 */
    if ((addr + len) > (APP_START_ADDR + APP_MAX_SIZE))
    {
        return IAP_INVALID_PARAM;
    }
    
    /* 解锁Flash */
    status = HAL_FLASH_Unlock();
    if (status != HAL_OK)
    {
        return IAP_FLASH_ERROR;
    }
    
    /* 按字（32位）写入 */
    while (remaining >= 4)
    {
        /* 组装32位数据（小端模式） */
        word_data = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
        
        /* 写入Flash */
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, write_addr, word_data);
        if (status != HAL_OK)
        {
            HAL_FLASH_Lock();
            return IAP_FLASH_ERROR;
        }
        
        /* 验证写入 */
        if (*(__IO uint32_t*)write_addr != word_data)
        {
            HAL_FLASH_Lock();
            return IAP_VERIFY_ERROR;
        }
        
        write_addr += 4;
        ptr += 4;
        remaining -= 4;
    }
    
    /* 处理剩余不足4字节的数据 */
    if (remaining > 0)
    {
        word_data = 0xFFFFFFFF;  /* Flash擦除后的默认值 */
        for (i = 0; i < remaining; i++)
        {
            ((uint8_t*)&word_data)[i] = ptr[i];
        }
        
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, write_addr, word_data);
        if (status != HAL_OK)
        {
            HAL_FLASH_Lock();
            return IAP_FLASH_ERROR;
        }
    }
    
    /* 锁定Flash */
    HAL_FLASH_Lock();
    
    return IAP_SUCCESS;
}

/**
  * @brief  读取Flash
  * @param  addr: 源地址
  * @param  data: 数据缓冲区
  * @param  len: 数据长度（字节）
  * @retval IAP状态
  */
IAP_StatusTypeDef Bootloader_Flash_Read(uint32_t addr, uint8_t *data, uint32_t len)
{
    uint32_t i;
    
    /* 参数检查 */
    if (!IS_VALID_FLASH_ADDR(addr) || data == NULL || len == 0)
    {
        return IAP_INVALID_PARAM;
    }
    
    /* 直接从Flash读取 */
    for (i = 0; i < len; i++)
    {
        data[i] = *(__IO uint8_t*)(addr + i);
    }
    
    return IAP_SUCCESS;
}

/**
  * @brief  开始固件升级
  * @retval IAP状态
  */
IAP_StatusTypeDef IAP_StartUpdate(void)
{
    IAP_StatusTypeDef status;
    
    /* 擦除APP区域 */
    status = Bootloader_Flash_Erase(APP_START_ADDR, APP_MAX_SIZE);
    if (status != IAP_SUCCESS)
    {
        return status;
    }
    
    /* 设置升级进行中标志 */
    update_in_progress = 1;
    
    /* 设置升级标志（防止断电后再次启动直接跳转到不完整的APP） */
    Bootloader_SetUpdateFlag();
    
    return IAP_SUCCESS;
}

/**
  * @brief  写入固件数据
  * @param  addr: 目标地址
  * @param  data: 数据缓冲区
  * @param  len: 数据长度
  * @retval IAP状态
  */
IAP_StatusTypeDef IAP_WriteData(uint32_t addr, uint8_t *data, uint32_t len)
{
    /* 检查是否已经开始升级 */
    if (!update_in_progress)
    {
        return IAP_ERROR;
    }
    
    /* 写入数据 */
    return Bootloader_Flash_Write(addr, data, len);
}

/**
  * @brief  结束固件升级
  * @retval IAP状态
  */
IAP_StatusTypeDef IAP_EndUpdate(void)
{
    /* 清除升级进行中标志 */
    update_in_progress = 0;
    
    /* 验证APP是否有效 */
    if (!Bootloader_CheckAppValid())
    {
        return IAP_APP_INVALID;
    }
    
    /* 清除升级标志 */
    Bootloader_ClearUpdateFlag();
    
    return IAP_SUCCESS;
}

/**
  * @brief  验证APP固件
  * @param  expected_crc: 期望的CRC32值
  * @retval IAP状态
  */
IAP_StatusTypeDef IAP_VerifyApp(uint32_t expected_crc)
{
    uint32_t calculated_crc;
    
    /* 计算APP的CRC32 */
    calculated_crc = Bootloader_GetAppCRC32();
    
    /* 比较CRC */
    if (calculated_crc != expected_crc)
    {
        return IAP_VERIFY_ERROR;
    }
    
    return IAP_SUCCESS;
}

/**
  * @brief  设置升级标志
  * @retval None
  */
void Bootloader_SetUpdateFlag(void)
{
    HAL_StatusTypeDef status;
    
    /* 解锁Flash */
    HAL_FLASH_Unlock();
    
    /* 写入升级标志 */
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, UPDATE_FLAG_ADDR, UPDATE_FLAG_VALUE);
    
    /* 锁定Flash */
    HAL_FLASH_Lock();
}

/**
  * @brief  清除升级标志
  * @retval None
  */
void Bootloader_ClearUpdateFlag(void)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;
    
    /* 解锁Flash */
    HAL_FLASH_Unlock();
    
    /* 擦除包含升级标志的页 */
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = UPDATE_FLAG_ADDR;
    EraseInitStruct.NbPages = 1;
    HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
    
    /* 锁定Flash */
    HAL_FLASH_Lock();
}

/**
  * @brief  检查升级标志
  * @retval 1: 升级标志有效, 0: 升级标志无效
  */
uint8_t Bootloader_CheckUpdateFlag(void)
{
    uint32_t flag = *(__IO uint32_t*)UPDATE_FLAG_ADDR;
    return (flag == UPDATE_FLAG_VALUE) ? 1 : 0;
}

/**
  * @brief  计算数据的CRC32
  * @param  data: 数据缓冲区
  * @param  len: 数据长度
  * @retval CRC32值
  */
uint32_t Bootloader_CalculateCRC32(uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    uint32_t i;
    
    for (i = 0; i < len; i++)
    {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    
    return ~crc;
}

/**
  * @brief  获取APP的CRC32
  * @retval CRC32值
  */
uint32_t Bootloader_GetAppCRC32(void)
{
    uint32_t crc = 0xFFFFFFFF;
    uint32_t addr;
    uint8_t byte;
    
    /* 计算整个APP区域的CRC32 */
    for (addr = APP_START_ADDR; addr < (APP_START_ADDR + APP_MAX_SIZE); addr++)
    {
        byte = *(__IO uint8_t*)addr;
        crc = crc32_table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }
    
    return ~crc;
}

/**
  * @brief  Bootloader反初始化
  * @retval None
  */
void Bootloader_DeInit(void)
{
    /* 反初始化HAL库 */
    HAL_DeInit();
    
    /* 关闭所有中断 */
    __disable_irq();
    
    /* 关闭SysTick */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
}

/**
  * @brief  复位设备
  * @retval None
  */
void Bootloader_ResetDevice(void)
{
    /* 系统软件复位 */
    NVIC_SystemReset();
}

/**
  * @brief  获取状态字符串（用于调试）
  * @param  status: IAP状态
  * @retval 状态字符串
  */
const char* Bootloader_GetStatusString(IAP_StatusTypeDef status)
{
    switch (status)
    {
        case IAP_SUCCESS:       return "Success";
        case IAP_ERROR:         return "Error";
        case IAP_FLASH_ERROR:   return "Flash Error";
        case IAP_VERIFY_ERROR:  return "Verify Error";
        case IAP_TIMEOUT:       return "Timeout";
        case IAP_INVALID_PARAM: return "Invalid Parameter";
        case IAP_APP_INVALID:   return "APP Invalid";
        default:                return "Unknown";
    }
}

/**
  * @brief  保存APP固件版本信息到Flash
  * @param  version: 版本信息结构体指针
  * @retval IAP状态
  */
IAP_StatusTypeDef Bootloader_SaveAppVersion(Firmware_VersionTypeDef *version)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;
    uint32_t *src_ptr = (uint32_t*)version;
    uint32_t write_addr = FW_VERSION_INFO_ADDR;
    uint32_t write_len = sizeof(Firmware_VersionTypeDef);
    uint32_t i;
    
    /* 参数检查 */
    if (version == NULL)
    {
        return IAP_INVALID_PARAM;
    }
    
    /* 设置魔术字 */
    version->magic = FW_VERSION_MAGIC;
    
    /* 解锁Flash */
    status = HAL_FLASH_Unlock();
    if (status != HAL_OK)
    {
        return IAP_FLASH_ERROR;
    }
    
    /* 擦除版本信息页 */
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = FW_VERSION_INFO_ADDR;
    EraseInitStruct.NbPages = 1;
    status = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
    if (status != HAL_OK)
    {
        HAL_FLASH_Lock();
        return IAP_FLASH_ERROR;
    }
    
    /* 按字（32位）写入版本信息 */
    for (i = 0; i < (write_len + 3) / 4; i++)
    {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, write_addr, src_ptr[i]);
        if (status != HAL_OK)
        {
            HAL_FLASH_Lock();
            return IAP_FLASH_ERROR;
        }
        
        /* 验证写入 */
        if (*(__IO uint32_t*)write_addr != src_ptr[i])
        {
            HAL_FLASH_Lock();
            return IAP_VERIFY_ERROR;
        }
        
        write_addr += 4;
    }
    
    /* 锁定Flash */
    HAL_FLASH_Lock();
    
    return IAP_SUCCESS;
}

/**
  * @brief  从Flash读取APP固件版本信息
  * @param  version: 版本信息结构体指针
  * @retval IAP状态
  */
IAP_StatusTypeDef Bootloader_ReadAppVersion(Firmware_VersionTypeDef *version)
{
    Firmware_VersionTypeDef *flash_version;
    
    /* 参数检查 */
    if (version == NULL)
    {
        return IAP_INVALID_PARAM;
    }
    
    /* 读取Flash中的版本信息 */
    flash_version = (Firmware_VersionTypeDef*)FW_VERSION_INFO_ADDR;
    
    /* 检查魔术字是否有效 */
    if (flash_version->magic != FW_VERSION_MAGIC)
    {
        return IAP_ERROR;  /* 版本信息无效 */
    }
    
    /* 复制版本信息 */
    memcpy(version, flash_version, sizeof(Firmware_VersionTypeDef));
    
    return IAP_SUCCESS;
}

/**
  * @brief  检查APP固件版本信息是否有效
  * @retval 1: 有效, 0: 无效
  */
uint8_t Bootloader_IsAppVersionValid(void)
{
    Firmware_VersionTypeDef *version = (Firmware_VersionTypeDef*)FW_VERSION_INFO_ADDR;
    
    /* 检查魔术字 */
    if (version->magic != FW_VERSION_MAGIC)
    {
        return 0;
    }
    
    return 1;
}

/**
  * @brief  比较两个版本号
  * @param  ver1: 版本1
  * @param  ver2: 版本2
  * @retval 1: ver1 > ver2, 0: ver1 == ver2, -1: ver1 < ver2, -2: 参数错误
  */
int8_t Bootloader_CompareVersion(Firmware_VersionTypeDef *ver1, Firmware_VersionTypeDef *ver2)
{
    /* 参数检查 */
    if (ver1 == NULL || ver2 == NULL)
    {
        return -2;
    }
    
    /* 比较主版本号 */
    if (ver1->major > ver2->major)
    {
        return 1;
    }
    else if (ver1->major < ver2->major)
    {
        return -1;
    }
    
    /* 主版本号相同，比较次版本号 */
    if (ver1->minor > ver2->minor)
    {
        return 1;
    }
    else if (ver1->minor < ver2->minor)
    {
        return -1;
    }
    
    /* 次版本号相同，比较修订号 */
    if (ver1->patch > ver2->patch)
    {
        return 1;
    }
    else if (ver1->patch < ver2->patch)
    {
        return -1;
    }
    
    /* 版本号完全相同，比较编译日期 */
    if (ver1->build_date > ver2->build_date)
    {
        return 1;
    }
    else if (ver1->build_date < ver2->build_date)
    {
        return -1;
    }
    
    /* 编译日期相同，比较编译时间 */
    if (ver1->build_time > ver2->build_time)
    {
        return 1;
    }
    else if (ver1->build_time < ver2->build_time)
    {
        return -1;
    }
    
    /* 完全相同 */
    return 0;
}

/**
  * @brief  擦除APP固件版本信息
  * @retval None
  */
void Bootloader_EraseAppVersion(void)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;
    
    /* 解锁Flash */
    HAL_FLASH_Unlock();
    
    /* 擦除版本信息页 */
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = FW_VERSION_INFO_ADDR;
    EraseInitStruct.NbPages = 1;
    HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
    
    /* 锁定Flash */
    HAL_FLASH_Lock();
}
/*
 * @brief  解析MCU版本字符串
 * @param  ver_str: 版本字符串
 * @param  major: 主版本号
 * @param  minor: 次版本号
 * @param  patch: 修订号
 * @retval 0: 成功, -1: 失败
 */
int Bootloader_ParseMCUVer(const char *ver_str, uint8_t *major, uint8_t *minor, uint16_t *patch)
{
    int m1 = 0, m2 = 0, m3 = 0;
    const char *p = ver_str;
    char c;

    if (!ver_str || !major || !minor || !patch) return -1;

    // 解析第一个数字
    while ((c = *p) && c >= '0' && c <= '9') {
        m1 = m1 * 10 + (c - '0');
        p++;
    }
    if (*p != '.') return -1;
    p++;

    // 解析第二个数字
    while ((c = *p) && c >= '0' && c <= '9') {
        m2 = m2 * 10 + (c - '0');
        p++;
    }
    if (*p != '.') return -1;
    p++;

    // 解析第三个数字
    while ((c = *p) && c >= '0' && c <= '9') {
        m3 = m3 * 10 + (c - '0');
        p++;
    }
    if (*p != '\0') return -1;

    // 范围检查
    if (m1 < 0 || m1 > 99 || m2 < 0 || m2 > 99 || m3 < 0 || m3 > 99)
        return -1;

    *major = (uint8_t)m1;
    *minor = (uint8_t)m2;
    *patch = (uint16_t)m3;
    return 0;
}

/**
 * @brief  生成版本字符串 "x.x.x"
 * @param  major: 主版本号（0~99）
 * @param  minor: 次版本号（0~99）
 * @param  patch: 修订号（0~99）
 * @param  out_str: 输出字符串缓冲区，至少8字节
 * @retval 0成功，-1失败
 */
int Bootloader_MCUVerString(uint8_t major, uint8_t minor, uint8_t patch, char *out_str)
{
    if (!out_str) return -1;
    if (major > 99 || minor > 99 || patch > 99) return -1;

    // 手动实现itoa，兼容无sprintf环境
    out_str[0] = (major / 10) + '0';
    out_str[1] = (major % 10) + '0';
    out_str[2] = '.';
    out_str[3] = (minor / 10) + '0';
    out_str[4] = (minor % 10) + '0';
    out_str[5] = '.';
    out_str[6] = (patch / 10) + '0';
    out_str[7] = (patch % 10) + '0';
    out_str[8] = '\0';

    // 去除前导0（如01.02.03变成1.2.3）
    // 可选：如需保留两位数，注释掉下面这段
    int i = 0, j = 0;
    while (out_str[i] == '0' && i < 2) i++;
    if (i) { out_str[j++] = out_str[i++]; }
    else { out_str[j++] = out_str[0]; }
    out_str[j++] = '.';
    i = 3;
    if (out_str[i] == '0') i++;
    out_str[j++] = out_str[i++];
    out_str[j++] = '.';
    i = 6;
    if (out_str[i] == '0') i++;
    out_str[j++] = out_str[i++];
    out_str[j] = '\0';

    return 0;
}


/**
  * @brief  更新flash中的APP固件版本信息 只能在app程序中调用！！！
  * @retval None
  */
IAP_StatusTypeDef Bootloader_UpdateAppVersion(void)
{
    /* 读取flash中的版本信息 */
    Firmware_VersionTypeDef current_version;
    Bootloader_ReadAppVersion(&current_version);
    /* 从MCU_VER宏获取当前版本信息*/
    Firmware_VersionTypeDef new_version;
    Bootloader_ParseMCUVer(MCU_VER, &new_version.major, &new_version.minor, &new_version.patch);
    /*对比版本号*/
    if(Bootloader_CompareVersion(&new_version, &current_version) != 0)  //版本号不同，更新flash中的版本信息
    {
        /* 更新版本信息 */
        return Bootloader_SaveAppVersion(&new_version);
    }
    return IAP_SUCCESS;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  获取地址所在的Flash页号
  * @param  addr: Flash地址
  * @retval Flash页号
  */
static uint32_t GetFlashPage(uint32_t addr)
{
    return (addr - BOOTLOADER_START_ADDR) / FLASH_PAGE_SIZE;
}

/**
  * @brief  写入一个字到Flash（辅助函数）
  * @param  addr: 目标地址
  * @param  data: 数据
  * @retval HAL状态
  */
static HAL_StatusTypeDef FlashWriteWord(uint32_t addr, uint32_t data)
{
    return HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, data);
}

