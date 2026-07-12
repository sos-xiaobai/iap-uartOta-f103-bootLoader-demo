# STM32F103C8 + WBR1 涂鸦云 OTA 教程

本文说明如何基于本仓库的 `bootloader` 和 `application` 工程完成 MCU OTA：编译 APP、取出 Keil 生成的 `.bin` 文件、上传到涂鸦设备云端、验证并下发升级。

官方流程参考：<https://developer.tuya.com/cn/docs/iot/firmware-upgrade-operation-guide?id=K93ixsft1w3to>

## 目录

- [STM32F103C8 + WBR1 涂鸦云 OTA 教程](#stm32f103c8--wbr1-涂鸦云-ota-教程)
  - [目录](#目录)
  - [1. 先看当前代码里的关键数值](#1-先看当前代码里的关键数值)
  - [2. 编译 APP 前必须修改的参数](#2-编译-app-前必须修改的参数)
    - [2.1 修改涂鸦产品 PID](#21-修改涂鸦产品-pid)
    - [2.2 修改 APP 版本号](#22-修改-app-版本号)
    - [2.3 确认打开 MCU 固件升级宏](#23-确认打开-mcu-固件升级宏)
    - [2.4 确认 Keil Flash 和 RAM 地址](#24-确认-keil-flash-和-ram-地址)
  - [3. APP 代码里必须保留的几行](#3-app-代码里必须保留的几行)
    - [3.1 main.c 需要包含的头文件](#31-mainc-需要包含的头文件)
    - [3.2 main() 最开始设置中断向量表](#32-main-最开始设置中断向量表)
    - [3.3 初始化 Bootloader 接口和涂鸦协议](#33-初始化-bootloader-接口和涂鸦协议)
    - [3.4 主循环中必须处理涂鸦串口服务](#34-主循环中必须处理涂鸦串口服务)
    - [3.5 UART 接收中断必须把字节交给涂鸦 SDK](#35-uart-接收中断必须把字节交给涂鸦-sdk)
    - [3.6 收到 OTA 开始命令后复位进 Bootloader](#36-收到-ota-开始命令后复位进-bootloader)
  - [4. 在 Keil 中生成 APP 的 bin 文件](#4-在-keil-中生成-app-的-bin-文件)
    - [4.1 编译工程](#41-编译工程)
    - [4.2 确认 Keil 自动生成 bin](#42-确认-keil-自动生成-bin)
    - [4.3 取出 bin 文件](#43-取出-bin-文件)
    - [4.4 上传前自检](#44-上传前自检)
    - [4.5 本地调试 APP，不走 OTA](#45-本地调试-app不走-ota)
  - [5. 涂鸦开发者平台上传固件](#5-涂鸦开发者平台上传固件)
    - [5.1 进入产品固件升级页面](#51-进入产品固件升级页面)
    - [5.2 新增或选择 MCU 固件](#52-新增或选择-mcu-固件)
    - [5.3 上传 application.bin](#53-上传-applicationbin)
    - [5.4 保存固件版本](#54-保存固件版本)
  - [6. 验证固件并下发 OTA](#6-验证固件并下发-ota)
    - [6.1 添加验证设备](#61-添加验证设备)
    - [6.2 对测试设备下发升级](#62-对测试设备下发升级)
    - [6.3 验证通过后发布](#63-验证通过后发布)
  - [7. 设备端 OTA 时序](#7-设备端-ota-时序)
  - [8. 常见问题](#8-常见问题)
    - [8.1 云端看不到可升级版本](#81-云端看不到可升级版本)
    - [8.2 OTA 后设备不运行 APP](#82-ota-后设备不运行-app)
    - [8.3 OTA 下发后马上复位但不写入](#83-ota-下发后马上复位但不写入)
    - [8.4 编译后没有 application.bin](#84-编译后没有-applicationbin)
  - [9. 发布前检查清单](#9-发布前检查清单)

## 1. 先看当前代码里的关键数值

所有地址、大小、版本号都以代码为准，不以旧说明文档或网上示例为准。当前仓库使用的是 STM32F103C8 的 64KB Flash 分区方案。

| 项目 | 当前代码值 | 来源 |
| --- | --- | --- |
| Bootloader 起始地址 | `0x08000000` | `bootloader/bootloader/bootloader.h` |
| Bootloader 大小 | `0x00004000`，16KB | `BOOTLOADER_SIZE` |
| APP 起始地址 | `0x08004000` | `APP_START_ADDR` |
| APP 最大空间 | `0x0000BC00`，47KB | `APP_MAX_SIZE` |
| Flash 结束地址 | `0x08010000`，64KB | `FLASH_END_ADDR` |
| 升级标志地址 | `0x0800FC00` | `UPDATE_FLAG_ADDR` |
| 升级标志值 | `0xAA55AA55` | `UPDATE_FLAG_VALUE` |
| APP 状态保存地址 | `0x0800FC04` | `APP_STATUS_ADDR` |
| 固件版本信息地址 | `0x0800FD00` | `FW_VERSION_INFO_ADDR` |
| APP RAM 起始地址 | `0x20001000` | `application/MDK-ARM/application/application.sct` |
| APP RAM 大小 | `0x00004000` | `application/MDK-ARM/application/application.sct` |
| 当前涂鸦 PID | `fnloi1nio96zmmeb` | `application/WBR3_SDK/protocol.h` 的 `PRODUCT_KEY` |
| 当前 APP 版本 | `1.0.5` | `application/WBR3_SDK/protocol.h` 的 `MCU_VER` |
| OTA 包大小选择 | `PACKAGE_SIZE 0`，256 字节 | `application/WBR3_SDK/protocol.h` |
| OTA 接收处理缓存 | `WIFI_DATA_PROCESS_LMT 300` | `application/WBR3_SDK/protocol.h` |

注意：旧文档里出现过 `0x08010000` 作为 APP 起始地址，那不是当前代码的配置。当前 APP 必须从 `0x08004000` 开始。

当前 Flash 布局：

```text
0x08000000  +------------------------------+
            | Bootloader, 16KB             |
0x08004000  +------------------------------+
            | Application, max 0xBC00      |
            |                              |
0x0800FC00  +------------------------------+
            | Update Flag / APP Status     |
0x0800FD00  +------------------------------+
            | Firmware Version Info        |
0x08010000  +------------------------------+
```

## 2. 编译 APP 前必须修改的参数

每次准备发布新的 OTA 包前，至少检查下面几处。

### 2.1 修改涂鸦产品 PID

文件：`application/WBR3_SDK/protocol.h`

```c
#define PRODUCT_KEY "fnloi1nio96zmmeb"
```

如果你换了涂鸦产品，必须把这里改成涂鸦开发者平台中该产品的 `PID`。设备上报产品信息时会带这个字段，云端也会用它匹配产品和固件。

Bootloader 工程中也有一份 `bootloader/WBR3_SDK/protocol.h`，其中 `PRODUCT_KEY` 也要保持一致，避免设备复位进入 Bootloader 后云端识别不到同一个产品。

### 2.2 修改 APP 版本号

文件：`application/WBR3_SDK/protocol.h`

```c
#define MCU_VER "1.0.5"
```

上传到涂鸦云端的固件版本号必须和这里一致。要让已经运行 `1.0.5` 的设备收到升级，下一版应改成更高版本，例如：

```c
#define MCU_VER "1.0.6"
```

不要把同一个版本号重复上传给已经是该版本的设备，否则云端通常会判断为没有可升级版本，设备不会收到 OTA。

### 2.3 确认打开 MCU 固件升级宏

文件：`application/WBR3_SDK/protocol.h`

```c
#define SUPPORT_MCU_FIRM_UPDATE

#ifdef SUPPORT_MCU_FIRM_UPDATE
#define PACKAGE_SIZE 0
#endif
```

当前 `PACKAGE_SIZE 0` 表示涂鸦模块每包下发 256 字节。对应缓存当前配置为：

```c
#define WIFI_UART_RECV_BUF_LMT 128
#define WIFI_DATA_PROCESS_LMT 300
```

如果把 `PACKAGE_SIZE` 改成 512 或 1024，必须同步调大 `WIFI_DATA_PROCESS_LMT`，否则 OTA 数据帧会放不下。

### 2.4 确认 Keil Flash 和 RAM 地址

文件：`application/MDK-ARM/application/application.sct`

当前正确配置如下：

```text
LR_IROM1 0x08004000 0x0000BC00  {
  ER_IROM1 0x08004000 0x0000BC00  {
   *.o (RESET, +First)
   *(InRoot$$Sections)
   .ANY (+RO)
  }
  RW_IRAM1 0x20001000 0x00004000  {
   .ANY (+RW +ZI)
  }
}
```

在 Keil 中也要确认：

1. 打开 `application/MDK-ARM/application.uvprojx`。
2. 进入 `Project -> Options for Target -> Target`。
3. `IROM1 Start` 填 `0x08004000`，`Size` 填 `0x0000BC00`。
4. `IRAM1 Start` 填 `0x20001000`，`Size` 填 `0x00004000`。
5. 如果工程使用 Scatter File，确认指向 `application/MDK-ARM/application/application.sct`。

只要 APP 链接到了 `0x08000000` 或其他地址，Bootloader 就无法正确跳转，OTA 后也会启动失败。

## 3. APP 代码里必须保留的几行

当前工程已经加好了这些代码。移植到其他 APP 时，按下面位置加入。

### 3.1 main.c 需要包含的头文件

文件：`application/Core/Src/main.c`

```c
#include "bootloader.h"
#include "bootloader_uart.h"
#include "wifi.h"
```

### 3.2 main() 最开始设置中断向量表

必须放在 `HAL_Init()` 之前：

```c
int main(void)
{
    // 重点
    SCB->VTOR = APP_START_ADDR;

    HAL_Init();
    SystemClock_Config();
    /* ... */
}
```

`APP_START_ADDR` 当前等于 `0x08004000`。这里不要手写成 `0x08010000`。

### 3.3 初始化 Bootloader 接口和涂鸦协议

当前工程在 `USER CODE BEGIN 2` 中保留了这些调用：

```c
// bootloadr初始化
Bootloader_Init();
// 涂鸦串口初始化
__HAL_UART_FLUSH_DRREGISTER(&huart1);
__HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_RXNE);
__HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_TC);
HAL_UART_Receive_IT(&huart1, temp_wifi_uart_rx_buf, 1);
//bootloadr串口初始化 这里和涂鸦串口一致
Bootloader_UART_Init(&huart1);
// 涂鸦wifi协议初始化
wifi_protocol_init();
// 从flash里面更新版本号
Bootloader_UpdateAppVersion();
// 用于给WBR1上报设备版本
extern void product_info_update(void);
product_info_update();
```

其中：

- `wifi_protocol_init()` 初始化涂鸦 MCU SDK。
- `Bootloader_UpdateAppVersion()` 把 `MCU_VER` 解析后写入 Flash 版本信息区。
- `product_info_update()` 主动向 WBR1 上报 `PRODUCT_KEY` 和 `MCU_VER`，让云端知道当前设备版本。

### 3.4 主循环中必须处理涂鸦串口服务

当前工程在 `while (1)` 中调用：

```c
wifi_uart_service();
```

这个函数负责解析 WBR1 模块发来的云端命令，包括普通 DP 下发和 OTA 开始命令。不要长时间阻塞主循环。

### 3.5 UART 接收中断必须把字节交给涂鸦 SDK

当前工程的回调逻辑：

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        HAL_UART_IRQHandler(&huart1);
        uart_receive_input(temp_wifi_uart_rx_buf[0]);
        HAL_UART_Receive_IT(&huart1, temp_wifi_uart_rx_buf, 1);
    }
}
```

WBR1 当前走 `USART1`，如果硬件改了串口，也要同步修改 `MX_USARTx_UART_Init()`、`huartx` 和回调里的判断。

### 3.6 收到 OTA 开始命令后复位进 Bootloader

当前实现位于 `application/WBR3_SDK/system.c`：

```c
case UPDATE_START_CMD:
    Bootloader_SetUpdateFlag();
    __disable_irq();
    HAL_NVIC_SystemReset();
    break;
```

这套框架的设计是：APP 收到云端 OTA 开始命令后只设置升级标志并复位，真正擦写 Flash、接收升级包和结束升级由 Bootloader 完成。

## 4. 在 Keil 中生成 APP 的 bin 文件

### 4.1 编译工程

1. 打开 `application/MDK-ARM/application.uvprojx`。
2. 选择目标 `application`。
3. 点击 `Rebuild` 或按 `F7` 编译。
4. 编译前再次确认 `MCU_VER` 已改成要发布的版本。

### 4.2 确认 Keil 自动生成 bin

当前 Keil 工程已经配置了 After Build：

```text
fromelf.exe --bin --output application\application.bin application\application.axf
```

如果你的 Keil 工程没有生成 `.bin`，手动配置：

1. 打开 `Project -> Options for Target -> User`。
2. 勾选 `Run User Programs After Build/Rebuild`。
3. 在 `Run #1` 填入：

```text
fromelf.exe --bin --output application\application.bin application\application.axf
```

这条命令的路径是相对 `application/MDK-ARM` 目录的。

### 4.3 取出 bin 文件

当前 OTA 要上传的文件路径是：

```text
.\application\MDK-ARM\application\application.bin
```

不要上传 `.hex`，也不要上传 `.axf`。涂鸦云端下发给 MCU 的应是原始二进制 `.bin`。

### 4.4 上传前自检

上传前检查：

1. `.bin` 文件大小必须小于等于 `0x0000BC00`，也就是 48128 字节。
2. `.map` 文件里向量表地址应从 `0x08004000` 开始。
3. `.bin` 前 4 字节应是栈顶地址，通常类似 `0x2000xxxx`。
4. `.bin` 第 5 到第 8 字节应是复位向量，通常类似 `0x08004xxx`。
5. 版本号 `MCU_VER` 和涂鸦后台填写的固件版本完全一致。

### 4.5 本地调试 APP，不走 OTA

如果只是本地调试 `application` 程序，不经过 Bootloader，也不验证 OTA 流程，可以不修改链接脚本参数，按普通 STM32 工程方式直接把 APP 烧录进 MCU 后调试即可。

这种方式只用于快速调试 APP 业务逻辑，例如外设、按键、显示、DP 上报和串口通信。此时不需要按 OTA 分区把 APP 链接到 `0x08004000`，也不需要取出 `.bin` 上传云端。

需要注意：本地直接烧录调试通过，不等于 OTA 包一定可用。准备发布 OTA 固件时，仍然必须回到前面章节的配置，确保 APP 起始地址、APP 大小、中断向量表、`MCU_VER` 和 `.bin` 文件都符合当前 Bootloader 的要求。

## 5. 涂鸦开发者平台上传固件

涂鸦官方文档入口：<https://developer.tuya.com/cn/docs/iot/firmware-upgrade-operation-guide?id=K93ixsft1w3to>

下面图片来自涂鸦官方文档页面，后台菜单可能会随涂鸦平台更新而略有变化。实际操作时以当前平台显示的菜单为准。

### 5.1 进入产品固件升级页面

1. 登录涂鸦开发者平台。
2. 进入 `产品开发`。
3. 找到和 `PRODUCT_KEY` 对应的产品。
4. 进入产品详情后，找到 `设备固件升级` 或 `固件升级` 页面。

![涂鸦固件升级入口](https://images.tuyacn.com/content-platform/hestia/1628753673656e82a28c5.png)

### 5.2 新增或选择 MCU 固件

在固件管理页面中，选择新增固件或进入已有 MCU 固件条目。常见字段按下面填写：

| 字段 | 填写建议 |
| --- | --- |
| 固件类型 | 选择 `MCU` 或与当前产品 MCU SDK 对应的固件类型 |
| 固件名称 | 例如 `stm32f103_app` |
| 固件标识 | 使用平台已生成或当前产品已有的 MCU 固件标识 |
| 适用产品 | 选择当前 PID 对应产品 |
| 固件版本 | 填 `MCU_VER`，例如当前代码是 `1.0.5`，下一版可填 `1.0.6` |
| 固件文件 | 上传 `application.bin` |
| 版本说明 | 写清楚本次变更、硬件版本、注意事项 |

![新增固件示意](https://images.tuyacn.com/content-platform/hestia/1669686578b4e4c97cd9d.png)

### 5.3 上传 application.bin

选择本地文件：

```text
.\application\MDK-ARM\application\application.bin
```

上传后确认平台识别到文件大小。这里的文件大小应和本地 `.bin` 一致，并且不能超过 `0x0000BC00`。

![上传固件文件](https://images.tuyacn.com/content-platform/hestia/16534619801a264d88838.png)

### 5.4 保存固件版本

保存前再次核对：

- 云端固件版本等于 `application/WBR3_SDK/protocol.h` 的 `MCU_VER`。
- 固件文件是 `.bin`。
- 产品 PID 和设备实际上报的 `PRODUCT_KEY` 一致。
- 固件类型是 MCU 固件，不是模组固件。

![保存固件版本](https://images.tuyacn.com/content-platform/hestia/1669687607395f9fd915e.png)

## 6. 验证固件并下发 OTA

不要直接全量发布未验证的固件。推荐先使用涂鸦平台的验证设备或灰度发布能力。

### 6.1 添加验证设备

1. 在固件版本页面选择 `验证固件` 或类似入口。
2. 添加测试设备，通常填写设备 ID、虚拟 ID 或平台要求的设备标识。
3. 确认测试设备当前在线，且已经通过 `product_info_update()` 上报了当前 `MCU_VER`。

![添加验证设备](https://images.tuyacn.com/content-platform/hestia/162882282948d121da2bf.png)

### 6.2 对测试设备下发升级

1. 选择刚上传的固件版本。
2. 对验证设备下发升级。
3. 设备收到 `UPDATE_START_CMD` 后，APP 会设置 `UPDATE_FLAG_ADDR` 并复位。
4. Bootloader 检测升级标志后进入 OTA 流程，继续和 WBR1 模块通信接收固件包。
5. 固件写入完成后，Bootloader 校验 APP 有效性，清除升级标志并跳转到新 APP。
6. 新 APP 启动后调用 `Bootloader_UpdateAppVersion()` 和 `product_info_update()`，云端看到新版本。

![验证下发升级](https://images.tuyacn.com/content-platform/hestia/1628822865d38241e4cc2.png)

### 6.3 验证通过后发布

验证设备升级成功后，再选择发布方式：

- 灰度发布：先给少量设备或指定比例设备升级，适合正式产品。
- 全量发布：确认稳定后再推给全部符合条件的设备。
- 指定设备发布：适合内部测试或售后定向升级。

![灰度或全量发布](https://images.tuyacn.com/content-platform/hestia/1653462160b80714f7b1d.png)

发布时重点关注：

1. 目标设备当前版本必须低于新固件版本。
2. 设备必须在线，WBR1 能连接云端。
3. 升级策略如果是 App 提醒升级，需要用户在 App 里确认。
4. 如果是静默升级或强制升级，设备满足条件后会由云端触发。

![发布固件](https://images.tuyacn.com/content-platform/hestia/166968768237a25dd4586.png)

## 7. 设备端 OTA 时序

当前代码的升级时序如下：

```text
涂鸦云端发布新 MCU 固件
        |
        v
WBR1 模块收到升级任务
        |
        v
APP 的 wifi_uart_service() 解析到 UPDATE_START_CMD
        |
        v
Bootloader_SetUpdateFlag()
HAL_NVIC_SystemReset()
        |
        v
Bootloader 启动并检查 UPDATE_FLAG_ADDR
        |
        v
Bootloader 通过 WBR1 接收固件包并写入 APP 区
        |
        v
IAP_EndUpdate() 校验 APP 并清除升级标志
        |
        v
跳转到 0x08004000 的新 APP
        |
        v
新 APP 上报新的 MCU_VER
```

## 8. 常见问题

### 8.1 云端看不到可升级版本

优先检查：

- `MCU_VER` 没有升高，例如设备已经是 `1.0.5`，云端也上传 `1.0.5`。
- 涂鸦后台填写的固件版本和代码里的 `MCU_VER` 不一致。
- `PRODUCT_KEY` 和涂鸦产品 PID 不一致。
- APP 没有调用 `product_info_update()`。
- 设备未联网，WBR1 没有连接云端。

### 8.2 OTA 后设备不运行 APP

优先检查：

- APP 是否链接到 `0x08004000`。
- `SCB->VTOR = APP_START_ADDR;` 是否在 `HAL_Init()` 前执行。
- `.bin` 是否超过 `0x0000BC00`。
- 上传的是否为 `.bin`，不是 `.hex` 或 `.axf`。
- Bootloader 和 APP 的 `APP_START_ADDR`、`APP_MAX_SIZE` 是否一致。

### 8.3 OTA 下发后马上复位但不写入

优先检查：

- Bootloader 工程里的 WBR1 串口和 APP 是否使用同一个硬件串口。
- Bootloader 里的 `PRODUCT_KEY` 是否和 APP 一致。
- Bootloader 是否能在复位后继续回复 WBR1 的产品信息。
- WBR1 的供电是否稳定，复位过程中模块是否掉电。

### 8.4 编译后没有 application.bin

检查 Keil 的 `Options for Target -> User` 是否启用 After Build，并填入：

```text
fromelf.exe --bin --output application\application.bin application\application.axf
```

当前仓库的 `application.uvprojx` 已经配置这条命令。

## 9. 发布前检查清单

- [ ] Bootloader 已烧录到 `0x08000000`。
- [ ] APP 起始地址是 `0x08004000`。
- [ ] APP 大小不超过 `0x0000BC00`。
- [ ] `SCB->VTOR = APP_START_ADDR;` 已保留。
- [ ] `SUPPORT_MCU_FIRM_UPDATE` 已开启。
- [ ] `PRODUCT_KEY` 和涂鸦产品 PID 一致。
- [ ] `MCU_VER` 已改成新版本。
- [ ] Keil 生成的是 `application.bin`。
- [ ] 涂鸦后台上传的是 `.bin` 文件。
- [ ] 云端固件版本和 `MCU_VER` 一致。
- [ ] 先用验证设备升级成功，再灰度或全量发布。
