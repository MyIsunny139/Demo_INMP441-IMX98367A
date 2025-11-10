# MAX98367A 音量增益使用说明

## 功能概述

MAX98367A 组件现已支持软件音量增益控制，可以在不改变硬件的情况下调整音频输出音量。

## 配置说明

### 默认增益配置
系统会自动使用 `MAX98367A.h` 中定义的默认增益值：

```c
//? 音量增益配置（在 components/MAX98367A/MAX98367A.h 中）
//? 增益范围: 0.0 ~ 5.0 (0.0=静音, 1.0=原音量, 5.0=5倍音量)
#ifndef MAX98367A_DEFAULT_GAIN
#define MAX98367A_DEFAULT_GAIN    3.0f  // 修改此值可改变默认音量
#endif

//? 最大增益限制
#ifndef MAX98367A_MAX_GAIN
#define MAX98367A_MAX_GAIN        5.0f  // 修改此值可改变最大增益限制
#endif
```

**修改默认音量**: 直接编辑 `components/MAX98367A/MAX98367A.h` 中的 `MAX98367A_DEFAULT_GAIN` 值
- 改为 `1.0f` - 原音量
- 改为 `2.0f` - 2倍音量
- 改为 `3.0f` - 3倍音量（当前默认）
- 改为 `5.0f` - 5倍音量（最大）

## API 说明

### 1. 设置音量增益（运行时动态调整）
```c
void max98367a_set_gain(float gain);
```
- **参数**: `gain` - 增益值，范围 0.0 ~ 5.0
  - `0.0` = 静音
  - `0.5` = 半音量
  - `1.0` = 原音量
  - `2.0` = 2倍音量
  - `3.0` = 3倍音量
  - `5.0` = 5倍音量（最大）
- **说明**: 
  - 超出范围的值会被自动限制在 0.0~5.0 之间
  - 用于运行时动态调整音量
  - 如果只需固定音量，直接修改头文件中的 `MAX98367A_DEFAULT_GAIN` 即可

### 2. 获取当前增益
```c
float max98367a_get_gain(void);
```
- **返回**: 当前的增益值

### 3. 应用增益到音频数据
```c
void max98367a_apply_gain(void *data, size_t len);
```
- **参数**: 
  - `data` - 音频数据缓冲区（int32_t数组）
  - `len` - 数据长度（字节数）
- **说明**: 通常在音频处理任务中调用，自动应用当前增益设置

## 使用示例

### 示例 1: 使用默认音量（推荐）
```c
void app_main(void)
{
    app_init();
    
    // 系统会自动使用 MAX98367A_DEFAULT_GAIN (2.0f)
    // 无需手动调用 max98367a_set_gain()
    
    // ... 其他初始化代码
}
```

**修改默认音量**: 直接编辑 `components/MAX98367A/MAX98367A.h`:
```c
#define MAX98367A_DEFAULT_GAIN    1.5f  // 改为1.5倍音量
```

### 示例 2: 在 app_main 中覆盖默认音量（可选）
```c
void app_main(void)
{
    app_init();

    // 可选：如果需要在运行时覆盖默认值
    max98367a_set_gain(1.5f);

    // ... 其他初始化代码
}
```

### 示例 3: 动态调整音量
```c
// 增大音量
void increase_volume(void)
{
    float current = max98367a_get_gain();
    max98367a_set_gain(current + 0.1f);
}

// 减小音量
void decrease_volume(void)
{
    float current = max98367a_get_gain();
    max98367a_set_gain(current - 0.1f);
}

// 静音
void mute(void)
{
    max98367a_set_gain(0.0f);
}

// 恢复原音量
void unmute(void)
{
    max98367a_set_gain(1.0f);
}
```

### 示例 4: 在音频处理任务中应用增益
```c
void i2s_read_send_task(void *pvParameters)
{
    uint8_t buf[BUF_SIZE];
    size_t bytes_read = 0;
    size_t bytes_written = 0;
 
    while (1) 
    {
        if (i2s_channel_read(rx_handle, buf, BUF_SIZE, &bytes_read, 100) == ESP_OK)
        {
            // 应用音量增益（使用当前设置的增益值）
            max98367a_apply_gain(buf, bytes_read);
            
            // 写入播放
            i2s_channel_write(tx_handle, buf, bytes_read, &bytes_written, 100);
        }
    }
}
```

### 示例 5: 通过 WebSocket 远程控制音量
```c
void on_ws_message(const char *msg, size_t len)
{
    // 假设接收到 JSON: {"cmd":"volume", "gain":1.5}
    if (strstr(msg, "\"cmd\":\"volume\"") != NULL) {
        // 解析 gain 值
        float gain = 1.0f;
        sscanf(msg, "{\"cmd\":\"volume\",\"gain\":%f}", &gain);
        
        // 设置音量
        max98367a_set_gain(gain);
        
        ESP_LOGI("WS", "Volume set to: %.2f", gain);
    }
}
```

## 配置方式说明

### ✅ 推荐方式：修改头文件默认值（固定音量）
**配置位置**: `components/MAX98367A/MAX98367A.h`
```c
#ifndef MAX98367A_DEFAULT_GAIN
#define MAX98367A_DEFAULT_GAIN    2.0f  // 直接修改这里
#endif
```

**优点**: 
- ✅ 简单直接，一处修改全局生效
- ✅ 编译时确定，无运行时开销
- ✅ 代码更简洁清晰
- ✅ 适合大多数固定音量场景

**使用场景**: 项目音量固定，不需要动态调整

---

### 🔧 可选方式：运行时动态调整（灵活控制）
**使用方法**: 在代码中调用 API
```c
max98367a_set_gain(1.5f);  // 运行时设置
```

**优点**:
- ✅ 灵活，可以随时调整
- ✅ 支持用户控制、远程控制等场景
- ✅ 可以实现自动增益控制（AGC）

**使用场景**: 
- 用户音量控制界面
- 远程音量调节
- 自动增益算法
- 音频淡入淡出效果

---

### 📋 配置流程建议

1. **设置基础音量**: 修改 `MAX98367A.h` 中的 `MAX98367A_DEFAULT_GAIN` （必需）
2. **调整最大限制** (可选): 修改 `MAX98367A_MAX_GAIN` 改变增益上限（默认5.0）
3. **动态调整** (可选): 如需运行时调整，调用 `max98367a_set_gain()`
4. **应用增益**: 在音频任务中调用 `max98367a_apply_gain()` （必需）

## 默认增益值配置

### 在头文件中修改（当前项目配置）
`components/MAX98367A/MAX98367A.h`:
```c
#ifndef MAX98367A_DEFAULT_GAIN
#define MAX98367A_DEFAULT_GAIN    3.0f  // 当前默认为3倍音量
#endif

#ifndef MAX98367A_MAX_GAIN
#define MAX98367A_MAX_GAIN        5.0f  // 最大增益限制为5倍
#endif
```

**修改示例**:
- 原音量: `#define MAX98367A_DEFAULT_GAIN    1.0f`
- 2倍: `#define MAX98367A_DEFAULT_GAIN    2.0f`
- 3倍: `#define MAX98367A_DEFAULT_GAIN    3.0f` （当前）
- 5倍: `#define MAX98367A_DEFAULT_GAIN    5.0f` （最大）

**调整最大增益限制**:
```c
#define MAX98367A_MAX_GAIN        10.0f  // 允许最大10倍增益
```

## 性能说明

### 计算开销
- 增益为 1.0 时，自动跳过计算，**无性能损耗**
- 增益非 1.0 时，每个采样点需要一次乘法和移位运算
- 在 ESP32-S3 @ 240MHz 下，处理 2048 字节（512个采样点）约需 **50 微秒**

### 音质影响
- 增益 < 1.0：降低音量，不会产生失真
- 增益 = 1.0：无损透传
- 增益 1.0 ~ 3.0：适度提高音量，一般情况下不会失真
- 增益 > 3.0：大幅提高音量，信号强度高时可能产生削波失真（已做溢出保护）
- 增益 = 5.0：最大增益，适合输入信号非常小的场景

### 失真说明
当增益较大时（如 > 3.0），如果输入信号本身就很强，放大后可能超出 INT32 范围，系统会自动限幅到 INT32_MAX，这会产生削波失真（听起来像爆音）。

**建议**:
- 输入信号强: 使用 1.0 ~ 2.0 倍增益
- 输入信号中等: 使用 2.0 ~ 3.0 倍增益（推荐）
- 输入信号弱: 使用 3.0 ~ 5.0 倍增益
- 可以先从 1.0 开始，逐步增大到合适的音量

## 技术细节

### 增益算法
```c
output = input × gain
```

为避免浮点运算，实际使用定点数实现：
```c
temp = (sample × (gain × 256)) >> 8
```

### 溢出保护
当音频信号 × 增益 > INT32_MAX 时，自动限幅到 INT32_MAX，防止失真爆音。

## 注意事项

1. **增益设置是全局的**，影响所有通过 MAX98367A 播放的音频
2. **线程安全**：增益值是简单的 float 变量，多线程同时修改可能存在竞争
3. **实时调整**：可以在任何时候调用 `max98367a_set_gain()`，立即生效
4. **功耗影响**：增益计算会略微增加 CPU 负载，但影响很小

## 典型应用场景

- 🔊 用户音量控制
- 🎚️ 自动增益控制（AGC）
- 🔇 快速静音功能
- 📱 远程音量调节
- 🎵 音频淡入淡出效果

## 故障排查

### 问题：设置增益后无效果
- 检查是否在音频任务中调用了 `max98367a_apply_gain()`
- 确认增益值不是 1.0（1.0 时自动跳过处理）

### 问题：音量过大产生破音
- 降低增益值到 1.5 以下
- 检查输入音频是否本身就很大

### 问题：音量过小听不见
- 增大增益值到 1.5 ~ 2.0
- 检查硬件连接和麦克风灵敏度

## 版本信息

- **添加日期**: 2025-11-10
- **版本**: v1.0
- **兼容性**: ESP-IDF v5.4.2+
