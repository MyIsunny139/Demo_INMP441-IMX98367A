# 项目移植指南

## 项目概述
这是一个基于 ESP32-S3 的实时音频处理项目，支持音频采集、播放和 WebSocket 通信。

## 架构设计

### 模块化设计
项目采用低耦合的模块化设计，便于移植和扩展：

```
demo_INMP441/
├── main/                       # 主程序
│   ├── App_Init.h/c            # 系统初始化和应用配置 ⭐
│   └── demo_INMP441.c          # 主程序入口
└── components/                 # 独立组件（各组件配置在各自头文件中）
    ├── INMP441/                # 麦克风驱动（独立）
    ├── MAX98367A/              # 功放驱动（独立）
    ├── wifi_sta/               # WiFi组件（独立）
    └── wss_client/             # WebSocket客户端（独立）
```

### 配置管理
项目采用**分散配置管理**，每个模块的配置在各自的头文件中：

- **应用配置**（`main/App_Init.h`）：
  - LED GPIO 定义
  - 应用任务配置（优先级、栈大小）

- **WiFi 配置**（`components/wifi_sta/wifi_sta.h`）：
  - WiFi SSID、密码
  - WiFi 重试次数

- **WebSocket 配置**（`components/wss_client/wss_client.h`）：
  - WebSocket URI、超时、重连参数
  - WebSocket 任务配置（优先级、栈大小）

- **音频组件配置**（各组件头文件）：
  - `INMP441/INMP441.h`：麦克风引脚和 I2S 参数
  - `MAX98367A/MAX98367A.h`：功放引脚和 I2S 参数

这种设计使得组件完全独立，可以直接复用到其他项目中。

### 组件依赖
各组件已解除循环依赖，仅依赖 ESP-IDF 标准组件：

- `INMP441`：独立的 I2S 麦克风驱动，只依赖 `driver`
- `MAX98367A`：独立的 I2S 功放驱动，只依赖 `driver`
- `wifi_sta`：通用 WiFi STA 组件，依赖 `nvs_flash`、`esp_wifi`、`esp_netif`、`esp_event`
- `wss_client`：通用 WebSocket 客户端，只依赖 `driver`

## 移植步骤

### 1. 修改音频硬件配置
如需更改麦克风或功放的引脚，请分别修改对应组件的头文件：

**修改 INMP441 引脚**（`components/INMP441/INMP441.h`）：
```c
//? INMP441引脚配置，根据自己连线修改
#define INMP_SD     GPIO_NUM_5
#define INMP_SCK    GPIO_NUM_6
#define INMP_WS     GPIO_NUM_4
```

**修改 MAX98367A 引脚**（`components/MAX98367A/MAX98367A.h`）：
```c
//? MAX98357A引脚，根据自己连线修改
#define MAX_DIN     GPIO_NUM_8
#define MAX_BCLK    GPIO_NUM_3
#define MAX_LRC     GPIO_NUM_46
```

**修改音频参数**（同样在各组件头文件中）：
```c
//? I2S配置参数
#define INMP441_SAMPLE_RATE     44100   // 采样率
#define INMP441_DMA_FRAME_NUM   256     // DMA缓冲帧数
```

### 2. 修改 LED 配置
编辑 `main/App_Init.h`：

```c
//? LED状态指示灯GPIO
#define GPIO_OUTPUT_IO_0    10
```

### 3. 修改网络配置
**WiFi 配置**（`components/wifi_sta/wifi_sta.h`）：

```c
//? WiFi SSID和密码
#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASS       "YOUR_WIFI_PASSWORD"

//? WiFi最大重试次数
#define WIFI_MAX_RETRY  5
```

**WebSocket 配置**（`components/wss_client/wss_client.h`）：

```c
//? WebSocket服务器URI
#define WSS_URI         "ws://your-server:port"

//? WebSocket连接超时（秒）
#define WSS_TIMEOUT_SEC 10

//? WebSocket重连间隔（毫秒）
#define WSS_RETRY_DELAY_MS  3000

//? WebSocket最大重连次数
#define WSS_MAX_RETRY   5
```

### 4. 调整音频参数（可选）
如需修改音频采样率或缓冲区大小，请在**各组件头文件**中修改：

**INMP441**（`components/INMP441/INMP441.h`）：
```c
#define INMP441_SAMPLE_RATE     44100   // 采样率
#define INMP441_DMA_FRAME_NUM   256     // DMA缓冲帧数
#define INMP441_BIT_WIDTH       32      // 位宽
```

**MAX98367A**（`components/MAX98367A/MAX98367A.h`）：
```c
#define MAX98367A_SAMPLE_RATE     44100   // 采样率
#define MAX98367A_DMA_FRAME_NUM   256     // DMA缓冲帧数
#define MAX98367A_BIT_WIDTH       32      // 位宽
```

**注意**：两个组件的采样率、位宽、声道数必须一致，否则音频会失真。

### 5. 移植到其他平台
如需移植到非 ESP32 平台：

1. **替换 I2S 驱动**：修改 `components/INMP441/INMP441.c` 和 `components/MAX98367A/MAX98367A.c` 中的 I2S 初始化代码，适配目标平台的 I2S 接口
2. **替换 WiFi 驱动**：`components/wifi_sta` 是 ESP32 专用组件，需要替换为目标平台的 WiFi 驱动
3. **替换 WebSocket**：`components/wss_client` 使用标准 BSD socket，移植性较好
4. **保持配置独立**：各组件的配置在自己的头文件中，移植时只需修改对应文件

### 6. 仅使用部分功能
由于组件解耦，可以选择性使用：

- **仅使用麦克风**：只包含 `INMP441` 组件，注释掉 `i2s_tx_init()` 和播放相关代码
- **仅使用功放**：只包含 `MAX98367A` 组件，注释掉 `i2s_rx_init()` 和录音相关代码
- **不使用网络**：注释掉 `App_Init.c` 中的 `wifi_Init()` 和 `wss_Init()` 调用
- **仅使用 WiFi**：包含 `wifi_sta` 组件，不包含 `wss_client`
- **使用其他网络协议**：`wifi_sta` 提供基础 WiFi 连接，可在此基础上实现 HTTP、MQTT 等

## 组件说明

### wifi_sta 组件
通用的 WiFi STA 模式组件，提供简洁的 API：

```c
//? 配置结构体
wifi_sta_config wifi_config = {
    .ssid = "YOUR_SSID",
    .password = "YOUR_PASSWORD",
    .max_retry = 5  // 0表示无限重试
};

//? 初始化并连接WiFi
wifi_sta_init(&wifi_config);

//? 可选：注册回调函数
wifi_sta_set_connected_callback(on_connected);
wifi_sta_set_disconnected_callback(on_disconnected);

//? 获取连接状态
bool connected = wifi_sta_is_connected();

//? 断开连接
wifi_sta_stop();
```

特性：
- 自动重连机制（可配置重试次数）
- 支持连接状态回调
- 自动初始化 NVS、TCP/IP 栈、事件循环
- 完整的错误处理和日志输出

### wss_client 组件

### 核心分配
- **Core 0**：运行音频任务（优先级 15）和 LED 任务（优先级 5）
- **Core 1**：运行 WebSocket 任务（优先级 3）

### 优先级说明
```c
TASK_AUDIO_PRIORITY     15      // 最高优先级，保证实时性
TASK_LED_PRIORITY       5       // 中等优先级
TASK_WSS_PRIORITY       3       // 较低优先级，避免影响音频
```

### 修改任务配置
**应用任务配置**（`main/App_Init.h`）：

```c
//? 任务优先级
#define TASK_AUDIO_PRIORITY     15
#define TASK_LED_PRIORITY       5

//? 任务堆栈大小（字节）
#define TASK_AUDIO_STACK_SIZE   4096
#define TASK_LED_STACK_SIZE     2048
```

**WebSocket 任务配置**（`components/wss_client/wss_client.h`）：

```c
//? WebSocket任务优先级
#define TASK_WSS_PRIORITY       3

//? WebSocket任务堆栈大小（字节）
#define TASK_WSS_STACK_SIZE     8192
```

## 优化建议

### 实时性优化
- 音频任务已移除延迟，以最快速度运行
- DMA 缓冲帧数从 511 降至 256，减少延迟约 57%
- I2S 超时从 1000ms 降至 100ms

### 内存优化
- 音频缓冲区使用全局变量，避免占用任务栈
- 各任务栈大小已优化

### 网络优化
- WebSocket 发送间隔为 2 秒，降低网络任务负载
- 支持 DNS 和连接重试，提高稳定性

## 故障排查

### 编译错误
- 确保所有组件的 CMakeLists.txt 依赖配置正确
- 检查各组件头文件中的配置宏定义

### 音频问题
- 检查 GPIO 引脚配置是否正确（在各组件头文件中）
- 验证采样率和位宽设置
- 确保音频任务优先级最高

### WebSocket 连接失败
- 增加 WiFi 初始化后的延迟
- 检查服务器 URI 格式（只支持 ws://）
- 查看重试日志

## 版本历史

### v2.1（当前版本）
- 配置分散到各模块头文件
- 移除 project_config.h，提高模块独立性
- 各组件配置更加清晰和独立

### v2.0
- 解除组件循环依赖
- 统一配置管理
- 简化 LED 功能
- 优化实时性能

### v1.0
- 初始版本
- 基本音频功能
- WebSocket 通信
