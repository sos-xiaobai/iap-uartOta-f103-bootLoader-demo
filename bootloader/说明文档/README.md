# STM32 IAP Bootloader 项目

一个功能完整的STM32 IAP (In-Application Programming) Bootloader实现，支持固件在线升级。

## 📁 项目结构

```
bootloader_IAP/
├── bootloader/                    # Bootloader核心模块
│   ├── bootloader.h              # Bootloader头文件（核心接口）
│   ├── bootloader.c              # Bootloader实现（Flash操作、IAP流程）
│   ├── bootloader_uart.h         # UART通信协议头文件
│   └── bootloader_uart.c         # UART通信协议实现
├── Core/                         # STM32 HAL核心文件
│   ├── Inc/
│   │   ├── main.h               # 主头文件
│   │   └── ...
│   └── Src/
│       ├── main.c               # 主程序（已集成Bootloader）
│       └── ...
├── tools/                        # 上位机工具
│   └── iap_upload.py            # Python固件升级工具
├── IAP_使用说明.md               # 详细使用说明文档
└── README.md                     # 本文件
```

## ✨ 主要特性

- ✅ **完整的IAP功能**: 支持固件擦除、写入、验证、跳转
- ✅ **Flash安全操作**: 保护Bootloader区域，防止误操作
- ✅ **CRC32校验**: 确保固件完整性
- ✅ **断电保护**: 升级标志机制，防止升级过程断电导致的问题
- ✅ **模块化设计**: 代码结构清晰，易于移植和扩展
- ✅ **UART通信协议**: 提供完整的串口通信协议实现
- ✅ **Python上位机**: 提供易用的固件上传工具
- ✅ **详细文档**: 包含完整的使用说明和示例代码

## 🚀 快速开始

### 1. 硬件要求

- **MCU**: STM32F103C8 (64KB Flash, 64KB RAM)
- **通信接口**: UART（可扩展USB、CAN等）
- **其他**: 按键、LED等（可选）

### 2. 软件要求

- **IDE**: Keil MDK / STM32CubeIDE
- **HAL库**: STM32F1xx HAL
- **Python**: Python 3.6+ (用于上位机工具)
- **依赖**: pyserial (`pip install pyserial`)

### 3. Flash分区配置

| 区域 | 起始地址 | 大小 | 说明 |
|------|----------|------|------|
| **Bootloader** | 0x08000000 | 16KB | Bootloader程序 |
| **Application** | 0x08004000 | 47KB | 应用程序 |
| **Update Flag** | 0x0800FC00 | 1KB | 升级标志 |

### 4. 编译配置

#### Bootloader项目

修改链接脚本（`bootloader.sct`）:

```
LR_IROM1 0x08000000 0x00004000  ; 16KB
{
    ER_IROM1 0x08000000 0x00004000
    {
        *.o (RESET, +First)
        *(InRoot$$Sections)
        .ANY (+RO)
    }
    RW_IRAM1 0x20000000 0x00004000
    {
        .ANY (+RW +ZI)
    }
}
```

#### Application项目

1. 修改链接脚本（`application.sct`）:

```
LR_IROM1 0x08010000 0x00070000  ; 448KB
{
    ER_IROM1 0x08010000 0x00070000
    {
        *.o (RESET, +First)
        *(InRoot$$Sections)
        .ANY (+RO)
    }
    RW_IRAM1 0x20000000 0x00004000
    {
        .ANY (+RW +ZI)
    }
}
```

2. 在`main()`函数开始处设置中断向量表偏移:

```c
int main(void)
{
    SCB->VTOR = 0x08010000;  // 设置中断向量表偏移
    HAL_Init();
    // ...
}
```

## 📝 使用方法

### 方式1: 使用UART协议（推荐）

1. **在Bootloader中启用UART协议**

```c
#include "bootloader_uart.h"

// 初始化UART（在CubeMX中配置UART1）
MX_USART1_UART_Init();

// 初始化UART协议
Bootloader_UART_Init(&huart1);

// 主循环
while (1)
{
    Bootloader_UART_Process();  // 处理UART命令
}
```

2. **使用Python工具上传固件**

```bash
python tools/iap_upload.py -p COM3 -f application.bin
```

### 方式2: 自定义实现

直接使用Bootloader API：

```c
#include "bootloader.h"

// 1. 初始化
Bootloader_Init();

// 2. 开始升级
IAP_StartUpdate();

// 3. 写入固件数据
uint8_t data[1024];
uint32_t addr = APP_START_ADDR;
// ... 接收数据
IAP_WriteData(addr, data, sizeof(data));

// 4. 结束升级
IAP_EndUpdate();

// 5. 跳转到APP
Bootloader_JumpToApp();
```

## 🔧 核心API

### Bootloader管理

```c
void Bootloader_Init(void);                              // 初始化
void Bootloader_GetInfo(Bootloader_InfoTypeDef *info);  // 获取信息
uint8_t Bootloader_CheckAppValid(void);                 // 检查APP有效性
void Bootloader_JumpToApp(void);                        // 跳转到APP
```

### Flash操作

```c
IAP_StatusTypeDef Bootloader_Flash_Erase(uint32_t addr, uint32_t size);
IAP_StatusTypeDef Bootloader_Flash_Write(uint32_t addr, uint8_t *data, uint32_t len);
IAP_StatusTypeDef Bootloader_Flash_Read(uint32_t addr, uint8_t *data, uint32_t len);
```

### IAP升级流程

```c
IAP_StatusTypeDef IAP_StartUpdate(void);                         // 开始升级
IAP_StatusTypeDef IAP_WriteData(uint32_t addr, ...);            // 写入数据
IAP_StatusTypeDef IAP_VerifyApp(uint32_t crc);                  // 验证固件
IAP_StatusTypeDef IAP_EndUpdate(void);                          // 结束升级
```

### 工具函数

```c
uint32_t Bootloader_CalculateCRC32(uint8_t *data, uint32_t len); // CRC32计算
void Bootloader_SetUpdateFlag(void);                              // 设置升级标志
void Bootloader_ClearUpdateFlag(void);                            // 清除升级标志
uint8_t Bootloader_CheckUpdateFlag(void);                         // 检查升级标志
```

## 📖 详细文档

更多详细信息请查看：

- **[IAP_使用说明.md](IAP_使用说明.md)** - 完整的使用说明文档
  - 系统架构
  - 通信协议详解
  - 配置说明
  - 常见问题
  - 扩展功能建议

## 🛠️ 自定义配置

在`bootloader.h`中可以修改以下配置：

```c
// Flash地址配置
#define BOOTLOADER_START_ADDR   0x08000000
#define BOOTLOADER_SIZE         0x4000  // 16KB
#define APP_START_ADDR          0x00004000
#define APP_MAX_SIZE            0x0000BC00  // 47KB

// Flash页大小（根据MCU型号修改）
#define FLASH_PAGE_SIZE         0x00000800  // 2KB

// 超时配置
#define IAP_TIMEOUT_MS          5000
```

## 📊 工作流程

```
┌─────────────────────────────────────────────────┐
│              上电/复位                          │
└───────────────┬─────────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────────┐
│        Bootloader_Init()                        │
│        初始化Bootloader                          │
└───────────────┬─────────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────────┐
│    检查升级标志/按键/串口命令                    │
└───────────────┬─────────────────────────────────┘
                │
        ┌───────┴───────┐
        │               │
        ▼               ▼
   需要升级          正常启动
        │               │
        │               ▼
        │    ┌─────────────────────┐
        │    │ Bootloader_JumpToApp│
        │    │    跳转到应用程序    │
        │    └─────────────────────┘
        │
        ▼
┌─────────────────────────────────────────────────┐
│              进入升级模式                        │
│                                                 │
│  1. IAP_StartUpdate() - 擦除APP区               │
│  2. IAP_WriteData() - 循环写入固件数据          │
│  3. IAP_VerifyApp() - 验证固件（可选）          │
│  4. IAP_EndUpdate() - 完成升级                  │
│  5. Bootloader_JumpToApp() - 跳转到新APP        │
└─────────────────────────────────────────────────┘
```

## 🔍 调试技巧

### 1. 使用串口输出调试信息

```c
Bootloader_UART_SendString("Bootloader v1.0.0\r\n");
Bootloader_UART_SendString("Waiting for firmware...\r\n");
```

### 2. LED指示

```c
// 闪烁LED表示Bootloader运行中
while (1)
{
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    Bootloader_UART_Process();
    HAL_Delay(100);
}
```

### 3. 查看状态码

```c
IAP_StatusTypeDef status = IAP_StartUpdate();
printf("Status: %s\n", Bootloader_GetStatusString(status));
```

## ⚠️ 注意事项

1. **Flash地址配置**: 确保Bootloader、Application和配置文件中的地址一致
2. **中断向量表**: Application必须设置`SCB->VTOR`
3. **链接脚本**: 必须正确配置两个项目的链接脚本
4. **写保护**: 确保Flash没有设置写保护
5. **调试器**: 使用SWD下载Bootloader后，断开调试器再测试跳转功能

## 🔄 移植到其他MCU

1. 修改`bootloader.h`中的地址和大小配置
2. 根据MCU的Flash页大小修改`FLASH_PAGE_SIZE`
3. 修改RAM范围检查常量：`APP_STACK_VALID_MIN`、`APP_STACK_VALID_MAX`
4. 根据HAL库版本调整Flash操作函数
5. 更新链接脚本

## 📜 许可证

本项目基于MIT许可证开源。

## 🤝 贡献

欢迎提交Issue和Pull Request！

## 📧 联系方式

如有问题或建议，欢迎交流！

---

**版本**: v1.0.0  
**日期**: 2025-11-05  
**作者**: Your Name  
**MCU**: STM32F103C8 (可移植到其他STM32系列)

