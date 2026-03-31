# 串口IAP升级使用说明

## 📋 功能说明

已在 `main.c` 中集成完整的串口IAP升级功能，包括：
- ✅ 自动初始化串口通信协议
- ✅ 启动信息显示
- ✅ APP有效性检查
- ✅ 自动跳转或进入IAP模式
- ✅ 完整的固件升级流程

## 🔌 硬件连接

| STM32引脚 | USB转串口模块 | 说明 |
|-----------|--------------|------|
| USART1_TX | RX | STM32发送 → PC接收 |
| USART1_RX | TX | STM32接收 ← PC发送 |
| GND | GND | 共地 |

**注意**：根据你的CubeMX配置，USART1可能对应不同的GPIO引脚（常见PA9/PA10）

## 🚀 使用步骤

### 1️⃣ 编译并下载Bootloader

1. 确保 `main.c` 中已包含：
   ```c
   #include "bootloader_uart.h"
   ```

2. 编译Bootloader项目（无错误和警告）

3. 下载到STM32板子（地址：0x08000000）

### 2️⃣ 打开串口助手查看启动信息

- **波特率**: 115200
- **数据位**: 8
- **停止位**: 1
- **校验位**: None

**首次启动时的串口输出**：
```
====================================
  STM32 IAP Bootloader v1.0.0
  MCU: STM32F103ZE
====================================
Application invalid!
Enter IAP mode...
IAP mode ready, waiting for firmware...
Use Python tool: python iap_upload.py -p COMx -f firmware.bin
```

### 3️⃣ 准备Application固件

确保你的Application项目：
- ✅ 链接脚本起始地址：`0x08010000`
- ✅ 在 `main()` 函数开始设置：`SCB->VTOR = 0x08010000;`
- ✅ 编译生成 `.bin` 文件

### 4️⃣ 使用Python工具上传固件

```bash
# 安装依赖（首次使用）
pip install pyserial

# 上传固件
python tools/iap_upload.py -p COM3 -f application.bin

# 显示详细信息
python tools/iap_upload.py -p COM3 -f application.bin -v
```

**升级过程示例**：
```
==================================================
STM32 IAP 固件升级工具
==================================================
✓ 读取固件文件: application.bin
  文件大小: 12345 字节
  CRC32: 0x12345678
✓ 串口连接成功: COM3 @ 115200

>>> 获取Bootloader信息...
  版本: 1.0.0
  Bootloader大小: 64 KB
  APP起始地址: 0x08010000
  APP最大大小: 448 KB
  MCU型号: STM32F103ZE

>>> 开始升级...
✓ 准备接收固件

>>> 写入固件 (12345 字节)...
  进度: |████████████████████████████████████████| 100.0% (13/13)
✓ 固件写入完成

>>> 结束升级...
✓ 升级完成

>>> 跳转到应用程序...
✓ 跳转成功

==================================================
固件升级成功！
==================================================
```

### 5️⃣ 验证升级结果

升级完成后，Bootloader会自动跳转到新的Application。

下次重启时：
```
====================================
  STM32 IAP Bootloader v1.0.0
  MCU: STM32F103ZE
====================================
Application valid, jumping...
```

然后自动运行新固件。

## 🎯 工作流程图

```
┌─────────────────┐
│   上电/复位      │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ 初始化Bootloader │
│ 初始化串口协议   │
│ 发送启动信息     │
└────────┬────────┘
         │
         ▼
   检查升级标志？
         │
    ┌────┴────┐
    │         │
   是        否
    │         │
    │         ▼
    │   检查APP有效？
    │         │
    │    ┌────┴────┐
    │    │         │
    │   有效      无效
    │    │         │
    │    ▼         │
    │  跳转到APP   │
    │    ↓         │
    └────┴─────────┘
         │
         ▼
   进入IAP模式
         │
         ▼
  ┌──────────────┐
  │ 主循环处理   │
  │ 串口IAP命令  │
  └──────────────┘
```

## 🛠️ 自定义配置

### 修改波特率

在 `usart.c` 的 `MX_USART1_UART_Init()` 函数中修改：
```c
huart1.Init.BaudRate = 115200;  // 修改为你需要的波特率
```

### 添加按键触发IAP模式

取消 `main.c` 中的注释：
```c
if (HAL_GPIO_ReadPin(BOOT_KEY_GPIO_Port, BOOT_KEY_Pin) == GPIO_PIN_RESET)
{
    Bootloader_UART_SendString("Button pressed, enter IAP mode...\r\n");
}
else
{
    // 尝试跳转到APP
    Bootloader_JumpToApp();
}
```

### 添加LED指示

取消 `main.c` 主循环中的注释：
```c
static uint32_t led_tick = 0;
if (HAL_GetTick() - led_tick > 500)
{
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    led_tick = HAL_GetTick();
}
```

## 📝 支持的IAP命令

| 命令码 | 命令名称 | 功能说明 |
|--------|---------|---------|
| 0xA4 | IAP_CMD_GET_INFO | 获取Bootloader信息 |
| 0xA5 | IAP_CMD_START_UPDATE | 开始升级（擦除APP区） |
| 0xA1 | IAP_CMD_WRITE | 写入固件数据 |
| 0xA6 | IAP_CMD_END_UPDATE | 结束升级 |
| 0xA7 | IAP_CMD_VERIFY | 校验固件CRC32 |
| 0xA3 | IAP_CMD_JUMP | 跳转到应用程序 |
| 0xA0 | IAP_CMD_ERASE | 擦除指定Flash区域 |
| 0xA2 | IAP_CMD_READ | 读取Flash数据 |

## 🐛 常见问题

### 1. 串口没有输出

**检查项**：
- ✅ USB转串口驱动是否安装
- ✅ 串口号是否正确（设备管理器查看）
- ✅ 波特率是否设置为115200
- ✅ TX/RX是否接反
- ✅ 是否共地（GND连接）

### 2. Python工具连接失败

```bash
# Windows查看可用串口
python -m serial.tools.list_ports

# 测试串口连接
python -c "import serial; s=serial.Serial('COM3',115200,timeout=1); print('OK')"
```

### 3. 上传失败：CRC校验错误

**原因**：串口通信干扰或波特率不匹配

**解决**：
- 降低波特率（如9600）
- 检查USB转串口模块质量
- 缩短连接线长度

### 4. 升级后无法启动APP

**检查清单**：
- [ ] Application链接脚本起始地址是否为 `0x08010000`
- [ ] Application是否设置了 `SCB->VTOR = 0x08010000;`
- [ ] Application是否编译生成 `.bin` 文件
- [ ] 固件文件是否完整

### 5. Bootloader自动跳转但APP不运行

在Application的 `main()` 函数**最开始**添加：
```c
int main(void)
{
    SCB->VTOR = 0x08010000;  // 必须在最前面！
    
    HAL_Init();
    // ... 其他代码
}
```

## 📊 串口数据包格式

### 请求包格式
```
+--------+-----+--------+----------+--------+--------+
| Header | CMD | Length | Address  |  Data  |  CRC32 |
| 2Bytes | 1B  | 2Bytes | 4Bytes   | N Bytes| 4Bytes |
+--------+-----+--------+----------+--------+--------+
  0xAA55   CMD   LEN      ADDR       DATA     CRC32
```

### 响应包格式
```
+--------+-----+--------+----------+--------+--------+--------+
| Header | CMD | Length | Address  | Status |  Data  | CRC32  |
| 2Bytes | 1B  | 2Bytes | 4Bytes   | 1Byte  | N Bytes| 4Bytes |
+--------+-----+--------+----------+--------+--------+--------+
  0xAA55   CMD   LEN      0x00      STATUS   DATA     CRC32
```

**状态码**：
- 0x00: 成功
- 0x01: 错误
- 0x02: CRC校验错误
- 0x03: Flash操作错误
- 0x04: 无效命令
- 0x05: 参数错误

## 💡 高级功能

### 手动发送命令（用于调试）

使用串口助手的16进制发送模式，可以手动发送命令测试。

**示例：获取Bootloader信息**
```
发送: AA 55 A4 00 00 00 00 00 00 [CRC32]
```

### 添加固件加密

在 `bootloader.c` 的 `IAP_WriteData()` 中添加解密代码：
```c
// 在写入前解密
for (uint32_t i = 0; i < len; i++)
{
    data[i] ^= 0x5A;  // 简单XOR解密
}
```

## 📚 相关文件

- `main.c` - Bootloader主程序
- `bootloader.c` - Bootloader核心功能
- `bootloader_uart.c` - 串口通信协议
- `tools/iap_upload.py` - Python上传工具

## 🎓 学习资源

- [IAP_使用说明.md](IAP_使用说明.md) - 完整使用文档
- [快速入门.md](快速入门.md) - 5分钟快速开始
- [README.md](README.md) - 项目总览

---

**祝你使用愉快！如有问题，欢迎查阅详细文档。**

