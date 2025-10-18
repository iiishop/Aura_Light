# Aura Light - 智能氛围灯光系统

## 项目概述

Aura Light 是一个基于 Arduino MKR WiFi 1010 的智能氛围灯光系统，集成了天气显示、时间跟随、音乐可视化和智能定时功能。系统通过 NeoPixel LED 灯带(8-16个灯珠)以灯光色彩的方式呈现信息，并提供基于 MQTT 的 Web Dashboard 进行远程控制和监控。

---

## 核心功能

### 1. 天气播报模式
- **触发方式**: 物理按钮按下
- **显示方式**: 通过灯光色彩组合表达天气状况
- **信息来源**: 实时天气 API

### 2. 呼吸灯模式(默认模式)
- **行为**: 平缓的呼吸效果
- **颜色变化**: 跟随时间变化(24小时色彩循环)
- **特点**: 低功耗、氛围营造

### 3. 智能定时器
- **启动**: 通过 Dashboard 设置定时
- **效果流程**:
  - 定时开始 → 所有灯珠全亮
  - 逐渐熄灭并色彩渐变
  - 定时结束 → 闪烁提醒
  - 按下按钮 → 停止闪烁

### 4. 音乐可视化(预留功能)
- **上传方式**: 通过 Dashboard 上传音频文件
- **分析**: 板载音频分析
- **效果**: 灯光跟随音乐节奏闪烁

### 5. Web Dashboard
- **架构**: MQTT Broker + HTML/JavaScript 客户端
- **功能**:
  - 实时天气详情显示
  - 定时器设置与管理
  - 音乐上传接口(预留)
  - 灯光模式切换
  - 系统状态监控

---

## 技术架构

### 系统架构图

```
┌─────────────────────────────────────────────────────────────┐
│                        Cloud Services                        │
│  ┌──────────────────┐         ┌──────────────────┐         │
│  │  Weather API     │         │  Time Server     │         │
│  │  (OpenWeather)   │         │  (NTP)           │         │
│  └──────────────────┘         └──────────────────┘         │
└────────────────┬─────────────────────┬──────────────────────┘
                 │                     │
                 └──────────┬──────────┘
                            │ WiFi
         ┌──────────────────┴──────────────────┐
         │       MQTT Broker (Mosquitto)       │
         │         (本地/云端部署)              │
         └──────────────────┬──────────────────┘
                            │ MQTT Protocol
         ┌──────────────────┼──────────────────┐
         │                  │                  │
    ┌────▼─────┐      ┌─────▼──────┐    ┌─────▼──────┐
    │ Arduino  │      │  Dashboard │    │   Mobile   │
    │MKR WiFi  │      │  (Browser) │    │  Clients   │
    │   1010   │      └────────────┘    └────────────┘
    └────┬─────┘
         │
    ┌────┴─────┐
    │ Hardware │
    │ ┌──────┐ │
    │ │Button│ │
    │ └──────┘ │
    │ ┌──────┐ │
    │ │Neo-  │ │
    │ │Pixels│ │
    │ └──────┘ │
    └──────────┘
```

### 通信协议

#### MQTT Topic 设计

```
aura-light/
├── status/
│   ├── online          # 设备在线状态
│   ├── mode            # 当前模式 (breathing/weather/timer/music)
│   └── error           # 错误信息
├── control/
│   ├── mode            # 模式切换命令
│   ├── timer/set       # 设置定时器
│   ├── timer/cancel    # 取消定时器
│   └── brightness      # 亮度调节
├── weather/
│   ├── current         # 当前天气数据
│   └── forecast        # 天气预报数据
├── time/
│   └── sync            # 时间同步
└── music/
    ├── upload          # 音乐数据上传
    └── control         # 播放控制
```

---

## 技术栈详解

### 硬件组件

| 组件 | 型号/规格 | 数量 | 用途 |
|------|----------|------|------|
| 主控板 | Arduino MKR WiFi 1010 | 1 | 核心控制器 |
| LED灯带 | WS2812B NeoPixel | 8-16颗 | 灯光显示 |
| 按钮 | 按键开关 | 1 | 模式切换/确认 |
| 电源 | 5V 2A电源适配器 | 1 | 供电 |
| 电阻 | 10kΩ 下拉电阻 | 1 | 按钮防抖 |
| 电容 | 100-1000μF | 1 | NeoPixel供电稳定 |

### 软件技术栈

#### Arduino 端

| 类别 | 库名称 | 版本 | 用途 |
|------|--------|------|------|
| **WiFi通信** | WiFiNINA | 1.8.14+ | WiFi连接管理 |
| **MQTT通信** | PubSubClient | 2.8+ | MQTT客户端功能 |
| **LED控制** | Adafruit NeoPixel | 1.12.0+ | NeoPixel LED控制 |
| **JSON解析** | ArduinoJson | 6.21.0+ | JSON数据处理 |
| **时间同步** | WiFiUdp | 内置 | NTP时间同步 |
| **HTTP客户端** | WiFiSSLClient | 内置 | HTTPS API请求 |
| **按钮防抖** | Bounce2 | 2.71+ | 按钮状态检测 |

#### Dashboard 端

| 技术 | 名称/库 | 版本 | 用途 |
|------|---------|------|------|
| **前端框架** | HTML5 + CSS3 + JavaScript | - | 页面结构 |
| **MQTT客户端** | Eclipse Paho MQTT.js | 1.18.0+ | 浏览器MQTT通信 |
| **UI框架** | 自定义CSS | - | 界面设计 |
| **图表库** | Chart.js | 4.4.0+ | 天气数据可视化 |
| **图标库** | Weather Icons | 2.0.12 | 天气图标显示 |

#### 后端服务

| 服务 | 名称 | 用途 |
|------|------|------|
| **MQTT Broker** | Mosquitto | 2.0.18+ | 消息代理 |
| **天气API** | OpenWeatherMap API | - | 天气数据获取 |
| **时间服务** | NTP Server | pool.ntp.org | 时间同步 |

---

## 详细实现步骤

### 阶段一: 硬件连接与基础测试 (第1-2天)

#### 步骤 1.1: NeoPixel LED 连接

**硬件连接:**
```
Arduino MKR WiFi 1010    →    NeoPixel LED Strip
─────────────────────────────────────────────────
Pin 6 (Data)             →    DIN (Data Input)
5V                       →    5V (Power)
GND                      →    GND (Ground)

注意事项:
- 在 NeoPixel 5V 和 GND 之间连接 100-1000μF 电容
- 在 Pin 6 和 DIN 之间可选添加 300-500Ω 电阻保护
- 确保电源能提供足够电流 (每颗LED约60mA)
```

**测试代码框架:**
```cpp
#include <Adafruit_NeoPixel.h>

#define LED_PIN 6
#define LED_COUNT 8

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.show(); // 初始化为全灭
}

void loop() {
  // 测试: 循环显示红、绿、蓝
}
```

**验收标准:**
- ✅ 所有 LED 可以独立控制
- ✅ 颜色显示准确
- ✅ 无闪烁或供电不足现象

#### 步骤 1.2: 按钮连接

**硬件连接:**
```
Arduino MKR WiFi 1010    →    按钮
────────────────────────────────────
Pin 2                    →    按钮一端
GND                      →    按钮另一端 (通过10kΩ下拉电阻)
```

**测试代码框架:**
```cpp
#include <Bounce2.h>

#define BUTTON_PIN 2

Bounce button = Bounce();

void setup() {
  button.attach(BUTTON_PIN, INPUT_PULLUP);
  button.interval(25); // 25ms 防抖
}

void loop() {
  button.update();
  if (button.fell()) {
    // 按钮被按下
  }
}
```

**验收标准:**
- ✅ 按钮按下能可靠检测
- ✅ 无误触发
- ✅ 防抖效果良好

---

### 阶段二: WiFi 与 MQTT 连接 (第3-4天)

#### 步骤 2.1: WiFi 连接配置

**配置文件: `arduino_secrets.h`**
```cpp
#define SECRET_SSID "你的WiFi名称"
#define SECRET_PASS "你的WiFi密码"
#define MQTT_BROKER "broker.hivemq.com"  // 或自建 Mosquitto
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PASS ""
```

**WiFi 连接实现步骤:**
1. 包含 WiFiNINA 库
2. 实现 WiFi 连接函数(带重连机制)
3. 添加连接状态 LED 指示
4. 实现连接失败处理

**关键函数:**
```cpp
void setupWiFi();
void reconnectWiFi();
bool isWiFiConnected();
```

#### 步骤 2.2: MQTT 连接实现

**MQTT 配置:**
```cpp
#include <PubSubClient.h>

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

const char* MQTT_CLIENT_ID = "aura-light-001";
```

**实现步骤:**
1. 初始化 MQTT 客户端
2. 实现连接函数
3. 设置回调函数处理接收消息
4. 实现自动重连机制
5. 订阅必要的 topic

**关键函数:**
```cpp
void setupMQTT();
void reconnectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void publishStatus(const char* status);
```

**验收标准:**
- ✅ WiFi 自动连接成功
- ✅ MQTT 连接到 Broker
- ✅ 能够发布和订阅消息
- ✅ 断线自动重连

---

### 阶段三: 核心功能实现 (第5-10天)

#### 步骤 3.1: 时间同步与时间色彩映射

**NTP 时间同步实现:**
```cpp
#include <WiFiUdp.h>

WiFiUDP Udp;
const char* ntpServer = "pool.ntp.org";
```

**实现步骤:**
1. 初始化 UDP 连接
2. 发送 NTP 请求
3. 解析 NTP 响应
4. 转换为本地时间
5. 每小时自动同步

**时间-颜色映射算法:**
```
时间范围 | 颜色方案 | RGB值范围
─────────|──────────|───────────
0-6时    | 深蓝紫   | (20-80, 0-40, 100-150)
6-8时    | 紫红渐变 | 渐变至 (150, 50, 80)
8-12时   | 金黄     | (200-255, 150-200, 50-100)
12-15时  | 明亮白   | (200-255, 200-255, 200-255)
15-18时  | 橙黄     | (255, 120-180, 30-80)
18-20时  | 橙红渐变 | (255, 80-120, 20-50)
20-24时  | 深蓝紫   | 渐变回 (20-80, 0-40, 100-150)
```

**关键函数:**
```cpp
void syncTime();
uint32_t getTimeBasedColor();
void applyTimeColor();
```

#### 步骤 3.2: 呼吸灯效果实现

**呼吸算法:**
- 使用正弦波函数实现平滑亮度变化
- 周期: 4-6秒完整呼吸循环
- 亮度范围: 20-255

**实现步骤:**
1. 定义呼吸周期参数
2. 使用 millis() 计算当前相位
3. 应用正弦函数计算亮度
4. 结合时间色彩更新 LED

**数学公式:**
```
brightness = minBrightness + (maxBrightness - minBrightness) * 
             (sin(2π * currentTime / period) + 1) / 2
```

**关键函数:**
```cpp
void breathingMode();
uint8_t calculateBreathingBrightness();
void updateBreathingLEDs();
```

#### 步骤 3.3: 天气 API 集成

**选择 API: OpenWeatherMap**
- 免费额度: 1000次/天
- 数据丰富: 当前天气、预报、多项指标
- API endpoint: `api.openweathermap.org/data/2.5/weather`

**实现步骤:**
1. 注册 OpenWeatherMap 账号获取 API Key
2. 实现 HTTPS GET 请求函数
3. 解析 JSON 响应 (使用 ArduinoJson)
4. 提取关键天气数据
5. 映射天气到颜色方案

**天气-颜色映射:**
```
天气状况       | 颜色方案           | LED效果
─────────────|───────────────────|──────────────
晴天 (Clear)  | 金黄色 (255,200,50) | 稳定明亮
多云 (Clouds) | 灰白色 (150,150,170)| 缓慢波动
雨天 (Rain)   | 蓝色 (50,100,200)   | 下落效果
雷暴 (Thunder)| 紫白闪烁            | 快速闪烁
雪天 (Snow)   | 冷白色 (200,220,255)| 闪烁飘落
雾霾 (Mist)   | 暗灰色 (80,80,90)   | 朦胧效果
```

**关键函数:**
```cpp
void fetchWeather();
void parseWeatherData(String json);
void displayWeatherEffect(String weatherCondition);
void weatherAnimation();
```

**数据结构:**
```cpp
struct WeatherData {
  String condition;      // 天气状况
  float temperature;     // 温度
  int humidity;          // 湿度
  float windSpeed;       // 风速
  String description;    // 描述
  long lastUpdate;       // 更新时间戳
};
```

#### 步骤 3.4: 天气播报按钮功能

**交互流程:**
1. 用户按下按钮
2. LED 显示"加载"动画(旋转彩虹)
3. 请求天气 API
4. 解析数据
5. 播放天气灯光效果(持续5-10秒)
6. 返回呼吸灯模式

**按钮中断处理:**
```cpp
volatile bool weatherButtonPressed = false;

void buttonISR() {
  weatherButtonPressed = true;
}
```

**关键函数:**
```cpp
void handleWeatherButton();
void showLoadingAnimation();
void playWeatherSequence();
```

---

### 阶段四: 定时器功能 (第11-13天)

#### 步骤 4.1: 定时器数据结构

**定时器设计:**
```cpp
struct Timer {
  bool active;           // 是否激活
  unsigned long duration; // 持续时间(毫秒)
  unsigned long startTime; // 开始时间
  bool alerting;         // 是否在提醒状态
};

Timer systemTimer;
```

#### 步骤 4.2: 定时器 MQTT 控制

**消息格式 (JSON):**
```json
// 设置定时器
{
  "action": "set",
  "duration": 1800  // 秒
}

// 取消定时器
{
  "action": "cancel"
}

// 停止提醒
{
  "action": "stop_alert"
}
```

**实现步骤:**
1. 订阅 `aura-light/control/timer/set`
2. 解析 JSON 消息
3. 启动定时器
4. 发布状态到 `aura-light/status/timer`

**关键函数:**
```cpp
void setTimer(unsigned long duration);
void cancelTimer();
void checkTimer();
void timerCompleteAlert();
```

#### 步骤 4.3: 定时器灯光效果

**效果序列:**
```
阶段 1: 全亮 (0-10%时间)
  - 所有 LED 白色全亮 (255,255,255)

阶段 2: 缓慢熄灭 (10-90%时间)
  - 亮度线性降低: 255 → 50
  - 颜色渐变: 白色 → 暖橙 → 深红

阶段 3: 最后阶段 (90-100%时间)
  - 极低亮度闪烁
  - 颜色: 深红色 (100,0,0)

阶段 4: 提醒闪烁 (时间结束后)
  - 快速闪烁: 0.5秒周期
  - 颜色: 红白交替
  - 持续直到按钮按下
```

**关键函数:**
```cpp
void timerLightSequence();
void calculateTimerColor(float progress);
void alertFlashing();
```

---

### 阶段五: Dashboard 开发 (第14-18天)

#### 步骤 5.1: MQTT Broker 部署

**选项 A: 本地 Mosquitto (推荐开发测试)**

**Windows 安装步骤:**
```powershell
# 下载 Mosquitto for Windows
# https://mosquitto.org/download/

# 安装后配置 mosquitto.conf
listener 1883
allow_anonymous true
listener 9001
protocol websockets

# 启动服务
net start mosquitto
```

**选项 B: 云端 Broker (推荐生产环境)**
- HiveMQ Cloud (免费层)
- CloudMQTT
- AWS IoT Core

**验证连接:**
```powershell
# 订阅测试
mosquitto_sub -h localhost -t test/topic

# 发布测试
mosquitto_pub -h localhost -t test/topic -m "Hello MQTT"
```

#### 步骤 5.2: Dashboard HTML 结构

**文件结构:**
```
dashboard/
├── index.html          # 主页面
├── css/
│   ├── style.css       # 主样式
│   └── weather-icons.css # 天气图标
├── js/
│   ├── mqtt-client.js  # MQTT 连接管理
│   ├── weather.js      # 天气显示逻辑
│   ├── timer.js        # 定时器控制
│   └── chart-config.js # 图表配置
└── assets/
    ├── icons/          # 图标资源
    └── fonts/          # 字体文件
```

**HTML 基础结构:**
```html
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Aura Light Dashboard</title>
    <link rel="stylesheet" href="css/style.css">
    <link rel="stylesheet" href="css/weather-icons.css">
</head>
<body>
    <!-- 导航栏 -->
    <nav id="navbar">
        <h1>Aura Light</h1>
        <div id="connection-status"></div>
    </nav>

    <!-- 主要内容区 -->
    <main>
        <!-- 天气信息卡片 -->
        <section id="weather-section">
            <h2>当前天气</h2>
            <div id="weather-display"></div>
            <canvas id="weather-chart"></canvas>
        </section>

        <!-- 定时器控制 -->
        <section id="timer-section">
            <h2>定时器</h2>
            <div id="timer-controls"></div>
            <div id="timer-display"></div>
        </section>

        <!-- 灯光模式切换 -->
        <section id="mode-section">
            <h2>灯光模式</h2>
            <div id="mode-buttons"></div>
        </section>

        <!-- 音乐上传(预留) -->
        <section id="music-section" style="opacity: 0.5;">
            <h2>音乐可视化 (即将推出)</h2>
            <input type="file" id="music-upload" accept="audio/*" disabled>
        </section>
    </main>

    <script src="https://unpkg.com/mqtt/dist/mqtt.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <script src="js/mqtt-client.js"></script>
    <script src="js/weather.js"></script>
    <script src="js/timer.js"></script>
    <script src="js/chart-config.js"></script>
</body>
</html>
```

#### 步骤 5.3: MQTT 客户端实现

**mqtt-client.js 核心代码:**
```javascript
// MQTT 连接配置
const MQTT_CONFIG = {
    host: 'broker.hivemq.com', // 或你的 Broker 地址
    port: 8000,
    protocol: 'ws', // WebSocket
    clientId: 'dashboard-' + Math.random().toString(16).substr(2, 8)
};

// 创建 MQTT 客户端
const client = mqtt.connect(`${MQTT_CONFIG.protocol}://${MQTT_CONFIG.host}:${MQTT_CONFIG.port}`, {
    clientId: MQTT_CONFIG.clientId,
    clean: true,
    reconnectPeriod: 1000
});

// 连接事件
client.on('connect', () => {
    console.log('MQTT Connected');
    subscribeToTopics();
    updateConnectionStatus(true);
});

// 订阅主题
function subscribeToTopics() {
    const topics = [
        'aura-light/status/#',
        'aura-light/weather/#',
        'aura-light/time/sync'
    ];
    topics.forEach(topic => {
        client.subscribe(topic);
    });
}

// 消息处理
client.on('message', (topic, message) => {
    const data = JSON.parse(message.toString());
    handleMessage(topic, data);
});

// 发布函数
function publishCommand(topic, payload) {
    client.publish(topic, JSON.stringify(payload));
}
```

#### 步骤 5.4: 定时器界面实现

**timer.js 核心功能:**
```javascript
// 定时器状态
let timerState = {
    active: false,
    duration: 0,
    remaining: 0,
    startTime: 0
};

// 设置定时器
function setTimer(minutes) {
    const payload = {
        action: 'set',
        duration: minutes * 60
    };
    publishCommand('aura-light/control/timer/set', payload);
}

// 取消定时器
function cancelTimer() {
    const payload = { action: 'cancel' };
    publishCommand('aura-light/control/timer/cancel', payload);
}

// 更新定时器显示
function updateTimerDisplay(data) {
    const remaining = data.remaining;
    const minutes = Math.floor(remaining / 60);
    const seconds = remaining % 60;
    document.getElementById('timer-display').textContent = 
        `${minutes.toString().padStart(2, '0')}:${seconds.toString().padStart(2, '0')}`;
}

// 预设时间按钮
const presetTimes = [5, 10, 15, 20, 25, 30, 45, 60];
function createTimerButtons() {
    const container = document.getElementById('timer-controls');
    presetTimes.forEach(time => {
        const btn = document.createElement('button');
        btn.textContent = `${time} 分钟`;
        btn.onclick = () => setTimer(time);
        container.appendChild(btn);
    });
}
```

#### 步骤 5.5: 天气显示界面

**weather.js 核心功能:**
```javascript
// 天气数据处理
function displayWeather(data) {
    const weatherDiv = document.getElementById('weather-display');
    
    const html = `
        <div class="weather-main">
            <i class="wi wi-${getWeatherIcon(data.condition)}"></i>
            <div class="temperature">${data.temperature}°C</div>
            <div class="condition">${data.description}</div>
        </div>
        <div class="weather-details">
            <div class="detail">
                <i class="wi wi-humidity"></i>
                <span>湿度: ${data.humidity}%</span>
            </div>
            <div class="detail">
                <i class="wi wi-strong-wind"></i>
                <span>风速: ${data.windSpeed} m/s</span>
            </div>
        </div>
    `;
    
    weatherDiv.innerHTML = html;
    updateWeatherChart(data);
}

// 天气图标映射
function getWeatherIcon(condition) {
    const iconMap = {
        'Clear': 'day-sunny',
        'Clouds': 'cloudy',
        'Rain': 'rain',
        'Thunderstorm': 'thunderstorm',
        'Snow': 'snow',
        'Mist': 'fog'
    };
    return iconMap[condition] || 'day-sunny';
}
```

#### 步骤 5.6: CSS 样式设计

**设计主题: 现代深色模式**

**色彩方案:**
```css
:root {
    /* 主色调 */
    --primary-color: #6C63FF;      /* 紫蓝色 */
    --secondary-color: #4CAF50;    /* 绿色 */
    --accent-color: #FF6B6B;       /* 红色 */
    
    /* 背景色 */
    --bg-primary: #0F0F1E;         /* 深蓝黑 */
    --bg-secondary: #1A1A2E;       /* 深灰蓝 */
    --bg-card: #16213E;            /* 卡片背景 */
    
    /* 文字色 */
    --text-primary: #E4E4E4;       /* 主文字 */
    --text-secondary: #A0A0A0;     /* 次要文字 */
    
    /* 边框和阴影 */
    --border-color: #2E2E3E;
    --shadow: 0 8px 32px rgba(0, 0, 0, 0.4);
}

/* 全局样式 */
body {
    margin: 0;
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    background: linear-gradient(135deg, var(--bg-primary) 0%, var(--bg-secondary) 100%);
    color: var(--text-primary);
    min-height: 100vh;
}

/* 卡片样式 */
section {
    background: var(--bg-card);
    border-radius: 16px;
    padding: 24px;
    margin: 16px;
    box-shadow: var(--shadow);
    border: 1px solid var(--border-color);
    transition: transform 0.3s ease, box-shadow 0.3s ease;
}

section:hover {
    transform: translateY(-4px);
    box-shadow: 0 12px 48px rgba(108, 99, 255, 0.2);
}

/* 按钮样式 */
button {
    background: linear-gradient(135deg, var(--primary-color) 0%, #5848C2 100%);
    color: white;
    border: none;
    padding: 12px 24px;
    border-radius: 8px;
    font-size: 16px;
    cursor: pointer;
    transition: all 0.3s ease;
    box-shadow: 0 4px 12px rgba(108, 99, 255, 0.3);
}

button:hover {
    transform: scale(1.05);
    box-shadow: 0 6px 20px rgba(108, 99, 255, 0.5);
}

/* 响应式设计 */
@media (max-width: 768px) {
    main {
        padding: 8px;
    }
    
    section {
        margin: 8px;
        padding: 16px;
    }
}
```

---

### 阶段六: 集成与优化 (第19-21天)

#### 步骤 6.1: Arduino 完整代码结构

**主程序文件组织:**
```
Aura_Light.ino          # 主程序
├── config.h            # 配置常量
├── hardware.h          # 硬件引脚定义
├── wifi_manager.h      # WiFi管理
├── mqtt_handler.h      # MQTT处理
├── led_effects.h       # LED效果库
├── weather_api.h       # 天气API
├── timer_manager.h     # 定时器管理
└── arduino_secrets.h   # 密钥配置
```

**主循环结构:**
```cpp
void loop() {
    // 1. 网络连接维护
    if (!WiFi.status() == WL_CONNECTED) {
        reconnectWiFi();
    }
    if (!mqttClient.connected()) {
        reconnectMQTT();
    }
    mqttClient.loop();
    
    // 2. 按钮检测
    button.update();
    if (button.fell()) {
        handleButtonPress();
    }
    
    // 3. 定时器检查
    if (systemTimer.active) {
        checkTimer();
    }
    
    // 4. 根据当前模式更新LED
    switch (currentMode) {
        case MODE_BREATHING:
            breathingMode();
            break;
        case MODE_WEATHER:
            weatherAnimation();
            break;
        case MODE_TIMER:
            timerLightSequence();
            break;
        case MODE_ALERT:
            alertFlashing();
            break;
    }
    
    // 5. 定期任务
    static unsigned long lastSync = 0;
    if (millis() - lastSync > 3600000) { // 每小时
        syncTime();
        lastSync = millis();
    }
    
    delay(20); // 50 FPS
}
```

#### 步骤 6.2: 内存优化

**优化策略:**
1. **使用 PROGMEM 存储常量字符串**
```cpp
const char MQTT_TOPIC_STATUS[] PROGMEM = "aura-light/status/online";
```

2. **限制 JSON 文档大小**
```cpp
StaticJsonDocument<512> doc; // 固定大小,避免碎片
```

3. **清理不用的缓冲区**
```cpp
void processWeather(String json) {
    // 处理后立即清空
    json = "";
}
```

4. **避免 String 拼接,使用 char 数组**
```cpp
char buffer[128];
snprintf(buffer, sizeof(buffer), "R:%d G:%d B:%d", r, g, b);
```

#### 步骤 6.3: 电源管理

**功耗优化:**
```cpp
// 降低 WiFi 功耗
WiFi.lowPowerMode();

// 空闲时降低 LED 亮度
void idleLowPower() {
    strip.setBrightness(50); // 最大255
}

// 夜间模式 (23:00-6:00)
void nightMode() {
    if (hour >= 23 || hour < 6) {
        strip.setBrightness(20);
    }
}
```

#### 步骤 6.4: 错误处理与日志

**错误分类:**
```cpp
enum ErrorCode {
    ERR_NONE = 0,
    ERR_WIFI_CONNECT = 1,
    ERR_MQTT_CONNECT = 2,
    ERR_API_REQUEST = 3,
    ERR_JSON_PARSE = 4,
    ERR_LED_INIT = 5
};

void logError(ErrorCode code, const char* message) {
    Serial.print("ERROR [");
    Serial.print(code);
    Serial.print("]: ");
    Serial.println(message);
    
    // 通过 MQTT 发布错误
    publishError(code, message);
    
    // LED 指示错误 (红色闪烁)
    indicateError(code);
}
```

#### 步骤 6.5: 系统状态监控

**监控指标:**
```cpp
struct SystemStatus {
    bool wifiConnected;
    bool mqttConnected;
    int rssi;              // WiFi信号强度
    unsigned long uptime;   // 运行时间
    int freeMemory;        // 可用内存
    String currentMode;    // 当前模式
    float cpuUsage;        // CPU使用率(估算)
};

void publishSystemStatus() {
    StaticJsonDocument<256> doc;
    doc["wifi"] = WiFi.status() == WL_CONNECTED;
    doc["rssi"] = WiFi.RSSI();
    doc["uptime"] = millis() / 1000;
    doc["mode"] = getModeString(currentMode);
    doc["memory"] = freeMemory();
    
    char buffer[256];
    serializeJson(doc, buffer);
    mqttClient.publish("aura-light/status/system", buffer);
}
```

---

### 阶段七: 测试与调试 (第22-24天)

#### 步骤 7.1: 单元测试清单

**硬件测试:**
- [ ] NeoPixel 所有LED可独立控制
- [ ] 按钮按下检测准确率 100%
- [ ] 电源供电稳定(无重启)
- [ ] WiFi 连接速度 < 10秒
- [ ] MQTT 消息收发延迟 < 500ms

**功能测试:**
- [ ] 呼吸灯效果流畅(无卡顿)
- [ ] 时间色彩过渡自然
- [ ] 天气API调用成功率 > 95%
- [ ] 天气灯光效果准确
- [ ] 定时器计时误差 < 1秒
- [ ] 定时器灯光序列完整
- [ ] 按钮停止提醒响应 < 1秒

**Dashboard 测试:**
- [ ] MQTT 连接稳定
- [ ] 实时数据更新延迟 < 2秒
- [ ] 定时器设置立即生效
- [ ] 天气数据显示准确
- [ ] 移动端响应式布局正常
- [ ] 多客户端同时连接无冲突

#### 步骤 7.2: 压力测试

**网络压力:**
```cpp
// 测试快速消息发送
for (int i = 0; i < 100; i++) {
    publishStatus("test");
    delay(50);
}
```

**长时间运行:**
- 连续运行 24 小时无重启
- 内存无泄漏(定期检查 freeMemory())
- WiFi 断线自动恢复

#### 步骤 7.3: 调试工具

**串口监控输出格式:**
```
[HH:MM:SS] [模块] 消息内容

示例:
[14:23:45] [WiFi] Connected to SSID: MyNetwork
[14:23:46] [MQTT] Connected to broker
[14:23:50] [Weather] API Response: Clear, 25°C
[14:24:00] [LED] Mode: Breathing, Brightness: 128
```

**Dashboard 调试控制台:**
```javascript
// 启用详细日志
const DEBUG = true;

function debugLog(module, message) {
    if (DEBUG) {
        console.log(`[${new Date().toLocaleTimeString()}] [${module}] ${message}`);
    }
}
```

---

## 音乐可视化功能(预留扩展)

### 未来实现方案

#### 选项 A: 板载音频分析(推荐)
**硬件需求:**
- MAX9814 麦克风模块
- 或 INMP441 MEMS 麦克风

**实现步骤:**
1. 连接麦克风到模拟输入
2. 采样音频信号(采样率: 8-16kHz)
3. FFT 分析(使用 arduinoFFT 库)
4. 提取频谱能量
5. 映射到 LED 效果

**关键库:**
- arduinoFFT v1.6+

#### 选项 B: Dashboard 音频上传分析
**实现步骤:**
1. 用户在 Dashboard 上传音频文件
2. JavaScript 使用 Web Audio API 分析
3. 提取节拍和频谱数据
4. 通过 MQTT 发送指令到 Arduino
5. Arduino 根据指令控制 LED

**优点:** 无需额外硬件
**缺点:** 实时性较差

---

## 部署指南

### Arduino 固件上传

**步骤:**
1. 打开 Arduino IDE
2. 安装必要库(通过库管理器):
   - WiFiNINA
   - PubSubClient
   - Adafruit NeoPixel
   - ArduinoJson
   - Bounce2
3. 配置 `arduino_secrets.h`
4. 选择开发板: Arduino MKR WiFi 1010
5. 选择端口
6. 点击上传

### Dashboard 部署

**选项 A: GitHub Pages(免费静态托管)**
```bash
# 推送到 GitHub
git init
git add dashboard/*
git commit -m "Initial dashboard"
git push origin main

# 在 GitHub 仓库设置中启用 Pages
# 访问: https://username.github.io/aura-light
```

**选项 B: 本地服务器**
```bash
# Python 简易服务器
cd dashboard
python -m http.server 8080

# 访问: http://localhost:8080
```

### MQTT Broker 生产配置

**Mosquitto 安全配置:**
```conf
# mosquitto.conf
listener 1883
allow_anonymous false
password_file /etc/mosquitto/passwd

listener 8883
cafile /etc/mosquitto/ca_certificates/ca.crt
certfile /etc/mosquitto/certs/server.crt
keyfile /etc/mosquitto/certs/server.key

listener 9001
protocol websockets
```

**创建用户:**
```bash
mosquitto_passwd -c /etc/mosquitto/passwd aura-light
mosquitto_passwd -b /etc/mosquitto/passwd dashboard dashboard_pass
```

---

## 故障排查

### 常见问题

| 问题 | 可能原因 | 解决方案 |
|------|----------|----------|
| LED 不亮 | 电源不足/接线错误 | 检查电源额定值,重新接线 |
| WiFi 无法连接 | SSID/密码错误 | 检查 arduino_secrets.h |
| MQTT 连接失败 | Broker地址错误/端口被封 | ping 测试,检查防火墙 |
| 天气API无响应 | API Key失效/网络问题 | 重新生成Key,检查网络 |
| Dashboard 无法连接 | WebSocket 未启用 | 检查Broker配置 |
| 定时器不准确 | NTP同步失败 | 手动同步,更换NTP服务器 |
| 内存溢出重启 | String拼接过多 | 优化代码,使用char数组 |

### 调试命令

**测试 MQTT 连接:**
```bash
# 订阅所有主题
mosquitto_sub -h broker_address -t "aura-light/#" -v

# 发布测试消息
mosquitto_pub -h broker_address -t "aura-light/control/mode" -m '{"mode":"breathing"}'
```

**测试天气 API:**
```bash
curl "https://api.openweathermap.org/data/2.5/weather?q=London&appid=YOUR_API_KEY"
```

---

## 性能指标

### 目标性能

| 指标 | 目标值 | 测量方法 |
|------|--------|----------|
| LED 刷新率 | 50 FPS | loop() 延迟 20ms |
| MQTT 延迟 | < 500ms | 发布到接收时间差 |
| 天气更新频率 | 每30分钟 | API调用日志 |
| 内存使用 | < 70% | freeMemory() |
| WiFi 重连时间 | < 10秒 | 断线到恢复时间 |
| 定时器精度 | ±1秒 | 对比系统时钟 |
| 功耗(待机) | < 2W | 功率计测量 |
| 功耗(全亮) | < 5W | 功率计测量 |

---

## 安全考虑

### 网络安全

1. **WiFi 安全:**
   - 使用 WPA2/WPA3 加密
   - 不在代码中硬编码密码
   - 定期更换密码

2. **MQTT 安全:**
   - 启用用户认证
   - 使用 TLS/SSL 加密(生产环境)
   - 限制 topic 访问权限

3. **API 密钥保护:**
   - 使用环境变量
   - 不提交到公开仓库
   - 定期轮换密钥

### 物理安全

- 确保电源连接牢固
- NeoPixel 散热考虑
- 外壳防尘防水(可选)

---

## 扩展功能建议

### 短期扩展(1-2周)

1. **多设备支持**
   - 每个设备唯一 ID
   - Dashboard 切换控制设备

2. **场景模式**
   - 预设场景: 阅读/放松/工作/派对
   - 一键切换

3. **通知功能**
   - 重要天气预警闪烁
   - 日历提醒

### 长期扩展(1-2月)

1. **语音控制**
   - 集成 Alexa/Google Assistant
   - 通过 MQTT 桥接

2. **自动化规则**
   - IFTTT 集成
   - 基于传感器自动调节

3. **数据分析**
   - 历史天气记录
   - 使用习惯分析

4. **移动应用**
   - React Native 开发
   - 推送通知

---

## 项目时间表

### 总计: 24 天开发周期

| 阶段 | 天数 | 里程碑 |
|------|------|--------|
| 阶段一: 硬件测试 | 2天 | 硬件连接完成 |
| 阶段二: 网络连接 | 2天 | WiFi+MQTT 工作 |
| 阶段三: 核心功能 | 6天 | 所有灯光模式实现 |
| 阶段四: 定时器 | 3天 | 定时器功能完成 |
| 阶段五: Dashboard | 5天 | Web界面可用 |
| 阶段六: 集成优化 | 3天 | 系统稳定 |
| 阶段七: 测试调试 | 3天 | 准备部署 |

---

## 物料清单(BOM)

| 组件 | 规格 | 数量 | 参考价格(CNY) |
|------|------|------|---------------|
| Arduino MKR WiFi 1010 | 官方正品 | 1 | ¥220 |
| WS2812B LED灯带 | 60灯/米,1米 | 1 | ¥35 |
| 按键开关 | 12mm 轻触开关 | 1 | ¥2 |
| 电源适配器 | 5V 3A | 1 | ¥25 |
| 电阻 | 10kΩ, 470Ω | 各1 | ¥1 |
| 电容 | 1000μF 16V | 1 | ¥3 |
| 面包板 | 标准尺寸 | 1 | ¥5 |
| 杜邦线 | 公对公/母 | 1套 | ¥8 |
| USB 数据线 | Micro USB | 1 | ¥10 |
| **总计** | | | **¥309** |

---

## 参考资源

### 官方文档

- [Arduino MKR WiFi 1010 文档](https://docs.arduino.cc/hardware/mkr-wifi-1010)
- [WiFiNINA 库文档](https://www.arduino.cc/reference/en/libraries/wifinina/)
- [Adafruit NeoPixel 指南](https://learn.adafruit.com/adafruit-neopixel-uberguide)
- [MQTT 协议规范](https://mqtt.org/mqtt-specification/)
- [OpenWeatherMap API 文档](https://openweathermap.org/api)

### 教程和示例

- [Arduino MQTT 教程](https://www.arduino.cc/en/Tutorial/mqtt)
- [NeoPixel 效果库](https://github.com/Aircoookie/WLED)
- [Chart.js 文档](https://www.chartjs.org/docs/)
- [Paho MQTT.js 示例](https://www.eclipse.org/paho/index.php?page=clients/js/index.php)

### 社区论坛

- [Arduino Forum](https://forum.arduino.cc/)
- [Reddit r/arduino](https://www.reddit.com/r/arduino/)
- [MQTT Community](https://mqtt.org/community)

---

## 许可证

本项目采用 MIT 许可证。

---

## 项目作者

**项目名称:** Aura Light  
**课程:** CASA0014 Connected Environments  
**学校:** UCL (University College London)  
**日期:** 2025年10月

---

## 版本历史

- **v1.0.0** (2025-10-18): 初始技术方案设计
- **v1.1.0** (待定): 实现基础功能
- **v1.2.0** (待定): Dashboard 完成
- **v2.0.0** (待定): 音乐可视化功能

---

## 致谢

感谢 Arduino 社区、Adafruit 和 MQTT.org 提供的优秀开源资源。

---

**文档结束**

如有疑问或需要技术支持,请查阅上述参考资源或在项目 Issues 中提问。
