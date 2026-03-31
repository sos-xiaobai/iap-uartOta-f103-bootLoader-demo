# STM32 IAP Bootloader 使用说明

## 目录
1. [概述](#概述)
2. [系统架构](#系统架构)
3. [Flash分区](#flash分区)
4. [核心功能](#核心功能)
5. [使用流程](#使用流程)
6. [配置说明](#配置说明)
7. [通信协议示例](#通信协议示例)
8. [注意事项](#注意事项)

---

## 概述

本项目实现了一个完整的STM32 IAP (In-Application Programming) Bootloader，支持固件在线升级功能。

### 主要特性

- ✅ 完整的Flash操作（擦除、写入、读取）
- ✅ APP有效性检查和安全跳转
- ✅ CRC32校验功能
- ✅ 升级标志管理（防止断电导致的问题）
- ✅ 模块化设计，易于集成
- ✅ 支持多种通信接口（UART/USB/CAN等）
- ✅ 详细的错误处理和状态返回

---

## 系统架构

```
+------------------+
|   Bootloader     |  (64KB: 0x08000000 - 0x0800FFFF)
|   启动、检测、   |
|   固件升级       |
+------------------+
|                  |
|   Application    |  (448KB: 0x08010000 - 0x0807F7FF)
|   用户应用程序   |
|                  |
+------------------+
|   Update Flag    |  (2KB: 0x0807F800 - 0x0807FFFF)
+------------------+
```

### 启动流程

```
上电复位
   ↓
Bootloader启动
   ↓
检查升级标志
   ↓
   ├─ 有标志 → 清除标志 → 进入升级模式
   │
   └─ 无标志 → 检查触发条件
                ↓
                ├─ GPIO按键按下 → 进入升级模式
                ├─ 串口命令 → 进入升级模式
                └─ 默认 → 检查APP有效性
                            ↓
                            ├─ APP有效 → 跳转到APP
                            └─ APP无效 → 进入升级模式
```

---

## Flash分区

### STM32F103ZE (512KB Flash)

| 区域 | 起始地址 | 大小 | 说明 |
|------|----------|------|------|
| **Bootloader** | 0x08000000 | 64KB | Bootloader程序 |
| **Application** | 0x08010000 | 448KB | 应用程序 |
| **Update Flag** | 0x0807F800 | 2KB | 升级标志区 |

### 配置说明

在 `bootloader.h` 中可以修改Flash分区配置：

```c
#define BOOTLOADER_START_ADDR   0x08000000
#define BOOTLOADER_SIZE         0x00010000  // 64KB
#define APP_START_ADDR          0x08010000  // Bootloader后
#define APP_MAX_SIZE            0x00070000  // 448KB
```

**注意**: 修改Flash分区后，需要同步修改：
1. Bootloader的链接脚本 (`.sct` 或 `.ld`)
2. Application的链接脚本（APP的起始地址）
3. Application的中断向量表偏移量 (`SCB->VTOR`)

---

## 核心功能

### 1. Bootloader初始化

```c
void Bootloader_Init(void);
```

初始化Bootloader，清除升级标志。在 `main()` 函数开始时调用。

### 2. APP检查和跳转

```c
// 检查APP是否有效
uint8_t Bootloader_CheckAppValid(void);

// 跳转到APP
void Bootloader_JumpToApp(void);
```

**APP有效性检查**:
- 栈顶地址在有效RAM范围内
- 复位向量在APP区域内
- 复位向量为Thumb指令（最低位为1）

### 3. Flash操作

```c
// 擦除Flash（按页擦除）
IAP_StatusTypeDef Bootloader_Flash_Erase(uint32_t start_addr, uint32_t size);

// 写入Flash（自动4字节对齐）
IAP_StatusTypeDef Bootloader_Flash_Write(uint32_t addr, uint8_t *data, uint32_t len);

// 读取Flash
IAP_StatusTypeDef Bootloader_Flash_Read(uint32_t addr, uint8_t *data, uint32_t len);
```

### 4. IAP升级流程

```c
// 1. 开始升级（擦除APP区）
IAP_StatusTypeDef IAP_StartUpdate(void);

// 2. 写入固件数据（可多次调用）
IAP_StatusTypeDef IAP_WriteData(uint32_t addr, uint8_t *data, uint32_t len);

// 3. 验证固件（可选）
IAP_StatusTypeDef IAP_VerifyApp(uint32_t expected_crc);

// 4. 结束升级
IAP_StatusTypeDef IAP_EndUpdate(void);
```

### 5. CRC校验

```c
// 计算数据的CRC32
uint32_t Bootloader_CalculateCRC32(uint8_t *data, uint32_t len);

// 获取整个APP的CRC32
uint32_t Bootloader_GetAppCRC32(void);
```

### 6. 升级标志管理

```c
// 设置升级标志（开始升级时调用）
void Bootloader_SetUpdateFlag(void);

// 清除升级标志（升级完成时调用）
void Bootloader_ClearUpdateFlag(void);

// 检查升级标志
uint8_t Bootloader_CheckUpdateFlag(void);
```

### 7. 其他工具函数

```c
// 获取Bootloader信息
void Bootloader_GetInfo(Bootloader_InfoTypeDef *info);

// 软件复位
void Bootloader_ResetDevice(void);

// 获取状态字符串（调试用）
const char* Bootloader_GetStatusString(IAP_StatusTypeDef status);
```

---

## 使用流程

### 完整的IAP升级示例

```c
#include "bootloader.h"

void IAP_Update_Example(void)
{
    IAP_StatusTypeDef status;
    uint8_t firmware_data[1024];
    uint32_t firmware_size = 0;
    uint32_t write_addr = APP_START_ADDR;
    uint32_t expected_crc = 0x12345678;  // 从上位机获取
    
    // 步骤1: 开始升级
    status = IAP_StartUpdate();
    if (status != IAP_SUCCESS)
    {
        // 处理错误
        return;
    }
    
    // 步骤2: 循环接收并写入固件数据
    while (还有数据需要接收)
    {
        // 从串口/USB等接收固件数据
        uint32_t len = 接收数据到firmware_data();
        
        // 写入Flash
        status = IAP_WriteData(write_addr, firmware_data, len);
        if (status != IAP_SUCCESS)
        {
            // 处理错误
            break;
        }
        
        write_addr += len;
        firmware_size += len;
    }
    
    // 步骤3: 验证固件（可选）
    status = IAP_VerifyApp(expected_crc);
    if (status != IAP_SUCCESS)
    {
        // CRC校验失败
        return;
    }
    
    // 步骤4: 结束升级
    status = IAP_EndUpdate();
    if (status != IAP_SUCCESS)
    {
        // 升级失败
        return;
    }
    
    // 步骤5: 跳转到新的APP
    HAL_Delay(100);
    Bootloader_JumpToApp();
}
```

---

## 配置说明

### 1. 修改Bootloader链接脚本

**Keil MDK** (`bootloader.sct`):

```
LR_IROM1 0x08000000 0x00010000  ; Bootloader占用64KB
{
    ER_IROM1 0x08000000 0x00010000
    {
        *.o (RESET, +First)
        *(InRoot$$Sections)
        .ANY (+RO)
    }
    RW_IRAM1 0x20000000 0x00010000
    {
        .ANY (+RW +ZI)
    }
}
```

### 2. 修改Application链接脚本

**Keil MDK** (`application.sct`):

```
LR_IROM1 0x08010000 0x00070000  ; APP从0x08010000开始，大小448KB
{
    ER_IROM1 0x08010000 0x00070000
    {
        *.o (RESET, +First)
        *(InRoot$$Sections)
        .ANY (+RO)
    }
    RW_IRAM1 0x20000000 0x00010000
    {
        .ANY (+RW +ZI)
    }
}
```

### 3. Application中设置中断向量表偏移

在Application的 `main()` 函数开始处添加：

```c
int main(void)
{
    // 设置中断向量表偏移到APP区域
    SCB->VTOR = 0x08010000;
    
    HAL_Init();
    // ... 其他初始化代码
}
```

或者在 `system_stm32f1xx.c` 中修改：

```c
#define VECT_TAB_OFFSET  0x00010000U  /* APP偏移量 */
```

### 4. 配置触发升级的条件

在 `main.c` 中根据需要配置进入升级模式的条件：

#### 方式1: GPIO按键触发

```c
// 初始化GPIO（在CubeMX中配置或手动初始化）
// 按键按下时进入升级模式
if (HAL_GPIO_ReadPin(BOOT_KEY_GPIO_Port, BOOT_KEY_Pin) == GPIO_PIN_RESET)
{
    // 进入升级模式，不跳转
}
else
{
    Bootloader_JumpToApp();
}
```

#### 方式2: 串口命令触发

```c
// 初始化UART
MX_USART1_UART_Init();

// 接收命令（带超时）
uint8_t cmd_buffer[10];
HAL_StatusTypeDef status = HAL_UART_Receive(&huart1, cmd_buffer, 10, 1000);

if (status == HAL_OK && memcmp(cmd_buffer, "IAP_UPDATE", 10) == 0)
{
    // 收到升级命令，进入升级模式
}
else
{
    Bootloader_JumpToApp();
}
```

---

## 通信协议示例

### 简单的串口协议

#### 数据包格式

```
+--------+--------+--------+----------+--------+--------+
| Header | Command| Length |  Address |  Data  |  CRC   |
|  2B    |  1B    |  2B    |   4B     |  N B   |  4B    |
+--------+--------+--------+----------+--------+--------+
  0xAA55    CMD     LEN      ADDR      DATA     CRC32
```

#### 命令定义

| 命令 | 值 | 说明 |
|------|-----|------|
| `IAP_CMD_GET_INFO` | 0xA4 | 获取Bootloader信息 |
| `IAP_CMD_START_UPDATE` | 0xA5 | 开始升级 |
| `IAP_CMD_WRITE` | 0xA1 | 写入数据 |
| `IAP_CMD_END_UPDATE` | 0xA6 | 结束升级 |
| `IAP_CMD_VERIFY` | 0xA7 | 校验固件 |
| `IAP_CMD_JUMP` | 0xA3 | 跳转到APP |

#### 示例：写入数据

**上位机 → STM32**:
```
AA 55 A1 00 04 00 01 00 08 [1024字节数据] [CRC32]
```

**STM32 → 上位机**:
```
AA 55 A1 00 01 00 [CRC32]  // 0x00表示成功
```

### Python上位机示例

```python
import serial
import struct

def send_firmware(port, firmware_path):
    ser = serial.Serial(port, 115200, timeout=5)
    
    # 读取固件文件
    with open(firmware_path, 'rb') as f:
        firmware = f.read()
    
    # 1. 开始升级
    send_command(ser, 0xA5, b'')
    
    # 2. 分包发送固件
    addr = 0x08010000
    chunk_size = 1024
    for i in range(0, len(firmware), chunk_size):
        chunk = firmware[i:i+chunk_size]
        send_write_command(ser, addr, chunk)
        addr += len(chunk)
        print(f"Progress: {(i+chunk_size)/len(firmware)*100:.1f}%")
    
    # 3. 结束升级
    send_command(ser, 0xA6, b'')
    
    # 4. 跳转到APP
    send_command(ser, 0xA3, b'')
    
    ser.close()
    print("Firmware update completed!")

def send_command(ser, cmd, data):
    # 构建数据包并发送
    # ... 实现细节
    pass
```

---

## 注意事项

### 1. Flash分区设置

⚠️ **重要**: Bootloader和Application的Flash地址配置必须保持一致：
- `bootloader.h` 中的地址定义
- Bootloader的链接脚本
- Application的链接脚本
- Application的中断向量表偏移 (`SCB->VTOR`)

### 2. 中断向量表

Application必须设置正确的中断向量表偏移量，否则中断将无法正常工作。

### 3. 升级安全性

- 升级过程中设置升级标志，防止断电导致的问题
- 支持CRC校验，确保固件完整性
- APP有效性检查，防止跳转到无效程序

### 4. Flash写入注意事项

- Flash必须先擦除后写入
- 写入地址必须4字节对齐
- 不能擦除或写入Bootloader区域

### 5. 通信接口选择

根据项目需求选择合适的通信接口：
- **UART**: 简单易用，速度较慢
- **USB**: 速度快，需要USB驱动
- **CAN**: 适合汽车电子，抗干扰强
- **以太网**: 远程升级

### 6. 调试技巧

```c
// 获取并打印Bootloader信息
Bootloader_InfoTypeDef info;
Bootloader_GetInfo(&info);
printf("Bootloader v%d.%d.%d\n", 
    info.version[0], info.version[1], info.version[2]);

// 获取状态字符串
IAP_StatusTypeDef status = IAP_StartUpdate();
printf("Status: %s\n", Bootloader_GetStatusString(status));
```

### 7. 常见问题

**问题1: 跳转到APP后程序不运行**
- 检查APP的起始地址是否正确
- 检查APP的中断向量表偏移是否设置
- 检查APP的栈顶地址是否有效

**问题2: Flash写入失败**
- 确认Flash已经擦除
- 确认地址在有效范围内
- 确认没有试图写入Bootloader区域

**问题3: 升级后APP无效**
- 使用CRC校验验证固件完整性
- 检查升级过程中是否有错误
- 确认固件文件是否正确

---

## 扩展功能建议

### 1. 添加备份功能

在升级前将旧固件备份到另一个Flash区域，升级失败时可以恢复。

### 2. 加密固件

对固件进行加密传输，在Bootloader中解密后写入Flash。

### 3. 差分升级

只传输固件的差异部分，减少升级时间。

### 4. 多APP管理

支持多个应用程序，可以选择启动不同的APP。

### 5. 远程升级

通过网络（以太网/WiFi/4G）实现远程固件升级。

---

## 联系方式

如有问题或建议，欢迎交流！

---

**版本**: v1.0.0  
**日期**: 2025-11-05  
**MCU**: STM32F103ZE  

