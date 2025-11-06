2025/11/6 添加websocket连接功能，目前具备的功能有
1. 实时音频采集与回放
INMP441 数字麦克风采集：通过 I2S 接口以 44.1kHz 采样率、32位精度采集音频
MAX98367A 功放输出：实时将采集的音频通过 I2S 输出到扬声器
低延迟直通模式：麦克风采集的音频直接传输到扬声器，延迟仅5ms
2. LED 和 PWM 可视化控制
LED 状态指示：通过 GPIO 10 控制 LED 开关
PWM 呼吸灯效果：GPIO 11 输出 PWM 信号（5kHz，12位分辨率）
自动渐变：占空比在 0-4095 之间循环渐变（2秒周期）
事件驱动：使用 FreeRTOS 事件组和队列实现异步控制
3. WiFi 网络连接
STA 模式：连接到指定的 WiFi AP（"MY_AP"）
自动重连：WiFi 断开时自动尝试重新连接
事件处理：监听 WiFi 连接、断开、获取 IP 等事件
配置持久化：通过 NVS 保存 WiFi 配置
4. WebSocket 网络通信
客户端连接：连接到远程 WebSocket 服务器（ws://124.222.6.60:8800）
定时发送：每秒向服务器发送 "hello" 消息
消息接收：接收并处理服务器返回的消息
双任务架构：
wss_send_task：专门负责发送消息
wss_recv_task：专门负责接收消息
连接管理：检测连接断开并进行清理

任务架构
Core 0 上的任务（音频处理）：
led_run_task（优先级 10）监听 LEDC 事件，控制 LED 和 PWM 渐变；
i2s_read_send_task（优先级 9）：I2S 音频数据读取，实时音频回放；

Core 1 上的任务（网络通信）：
wss_client_task（优先级 5）：WebSocket 连接初始化，连接管理和监控；
wss_send_task（优先级 5）：WebSocket 消息发送
wss_recv_task（优先级 5）：WebSocket 消息接收
