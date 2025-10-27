#include "luminaire_controller.h"
#include "music_mode.h"
#include "audio_analyzer.h"
#include <ArduinoJson.h>

LuminaireController::LuminaireController()
    : mqtt(nullptr),
      musicMode(nullptr),
      audioAnalyzer(nullptr),
      isActive(false),
      state(LUMI_OFF),
      mode(LUMI_MODE_IDLE),
      idleColor(0x0000FF), // 默认蓝色
      lastBreathUpdate(0),
      breathDirection(1),
      breathBrightness(0),
      currentTemp(20.0),
      feelsLikeTemp(20.0),
      humidity(50),
      windSpeed(0),
      windDirection("N"),
      visibility(10),
      cloudCover(0),
      precipitation(0.0),
      weatherCode("113"),
      weatherDesc("Sunny"),
      lastWeatherUpdate(0),
      lastWindUpdate(0),
      windAnimationOffset(0)
{

    memset(RGBpayload, 0, LUMINAIRE_PAYLOAD_SIZE);
}

void LuminaireController::begin(MQTTManager *mqttManager, const String &id)
{
    mqtt = mqttManager;
    lightId = id;
    mqttTopic = "student/CASA0014/luminaire/" + lightId;

    Serial.println("\n========================================");
    Serial.println("[Luminaire] Initializing Luminaire Controller");
    Serial.println("========================================");
    Serial.print("[Luminaire] Light ID: ");
    Serial.println(lightId);
    Serial.print("[Luminaire] MQTT Topic: ");
    Serial.println(mqttTopic);
    Serial.print("[Luminaire] Number of LEDs: ");
    Serial.println(LUMINAIRE_NUM_LEDS);
    Serial.println("========================================\n");
}

void LuminaireController::setMusicMode(MusicMode *music, AudioAnalyzer *audio)
{
    musicMode = music;
    audioAnalyzer = audio;
    Serial.println("[Luminaire] Music mode and audio analyzer configured");
}

void LuminaireController::loop()
{
    // 只在激活且开启时更新
    if (!isActive || state != LUMI_ON)
    {
        return;
    }

    // Music 模式更新
    if (mode == LUMI_MODE_MUSIC && musicMode != nullptr && audioAnalyzer != nullptr)
    {
        // 限制更新频率为每秒约 20 次（避免阻塞主循环）
        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate > 50) // 50ms = 20 FPS
        {
            updateMusicSpectrum();
            lastUpdate = millis();
        }
    }
    // IDLE 模式呼吸灯更新
    else if (mode == LUMI_MODE_IDLE)
    {
        updateBreathingEffect();
    }
    // WEATHER 模式天气可视化更新
    else if (mode == LUMI_MODE_WEATHER)
    {
        updateWeatherVisualization();
    }
}

void LuminaireController::setActive(bool active)
{
    isActive = active;
    Serial.print("[Luminaire] Controller ");
    Serial.println(active ? "ACTIVATED" : "DEACTIVATED");

    if (!active)
    {

        clear();
    }
}

void LuminaireController::sendRGBToPixel(int r, int g, int b, int pixel)
{
    if (!isActive)
    {
        Serial.println("[Luminaire] ✗ Controller not active");
        return;
    }

    if (pixel < 0 || pixel >= LUMINAIRE_NUM_LEDS)
    {
        Serial.print("[Luminaire] ✗ Invalid pixel: ");
        Serial.println(pixel);
        return;
    }

    // MQTT 连接检查已移至 updateMusicSpectrum() 开始处
    // 这里直接发送以避免重复检查和日志洪水

    RGBpayload[pixel * 3 + 0] = (byte)r;
    RGBpayload[pixel * 3 + 1] = (byte)g;
    RGBpayload[pixel * 3 + 2] = (byte)b;

    mqtt->publish(mqttTopic.c_str(), RGBpayload, LUMINAIRE_PAYLOAD_SIZE, false);

    // 日志已禁用（Music模式下太频繁）
    // Serial.print("[Luminaire] ✓ Sent RGB(");
}

void LuminaireController::sendRGBToAll(int r, int g, int b)
{
    if (!isActive)
    {
        Serial.println("[Luminaire] ✗ Controller not active");
        return;
    }

    if (!mqtt || !mqtt->isConnected())
    {
        Serial.println("[Luminaire] ✗ MQTT not connected");
        return;
    }

    for (int pixel = 0; pixel < LUMINAIRE_NUM_LEDS; pixel++)
    {
        RGBpayload[pixel * 3 + 0] = (byte)r;
        RGBpayload[pixel * 3 + 1] = (byte)g;
        RGBpayload[pixel * 3 + 2] = (byte)b;
    }

    mqtt->publish(mqttTopic.c_str(), RGBpayload, LUMINAIRE_PAYLOAD_SIZE, false);

    // 日志已禁用（Music模式下太频繁）
    // Serial.print("[Luminaire] ✓ Sent RGB(");
}

void LuminaireController::clear()
{
    if (!mqtt || !mqtt->isConnected())
    {
        return;
    }

    memset(RGBpayload, 0, LUMINAIRE_PAYLOAD_SIZE);

    mqtt->publish(mqttTopic.c_str(), RGBpayload, LUMINAIRE_PAYLOAD_SIZE, false);
    Serial.println("[Luminaire] ✓ All LEDs cleared");
}

void LuminaireController::handleMQTTMessage(char *topic, byte *payload, unsigned int length)
{

    String message = "";
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }

    Serial.print("[Luminaire] Received [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.println(message);

    String topicStr = String(topic);

    if (topicStr.endsWith("/status"))
    {
        if (message == "on" || message == "ON" || message == "1")
        {
            Serial.println("[Luminaire] Setting state to ON");
            state = LUMI_ON;

            // 如果是 IDLE 模式，初始化呼吸灯
            if (mode == LUMI_MODE_IDLE)
            {
                breathBrightness = 0;
                breathDirection = 1;
                lastBreathUpdate = millis();
            }

            applyModeColor();
        }
        else if (message == "off" || message == "OFF" || message == "0")
        {
            Serial.println("[Luminaire] Setting state to OFF");
            state = LUMI_OFF;
            clear();
        }
    }

    else if (topicStr.endsWith("/mode"))
    {
        message.toLowerCase();
        if (message == "timer")
        {
            mode = LUMI_MODE_TIMER;
        }
        else if (message == "weather")
        {
            mode = LUMI_MODE_WEATHER;
        }
        else if (message == "idle")
        {
            mode = LUMI_MODE_IDLE;
            // 初始化呼吸灯参数
            breathBrightness = 0;
            breathDirection = 1;
            lastBreathUpdate = millis();
        }
        else if (message == "music")
        {
            mode = LUMI_MODE_MUSIC;
        }

        const char *modeNames[] = {"timer", "weather", "idle", "music"};
        Serial.print("[Luminaire] Setting mode to: ");
        Serial.println(modeNames[mode]);

        if (state == LUMI_ON)
        {
            applyModeColor();
        }
    }

    else if (topicStr.endsWith("/debug/color"))
    {

        int colonPos = message.indexOf(':');
        if (colonPos > 0)
        {

            int index = message.substring(0, colonPos).toInt();
            String colorStr = message.substring(colonPos + 1);
            int r, g, b;
            getRGBFromHex(colorStr, r, g, b);
            sendRGBToPixel(r, g, b, index);
        }
        else
        {

            int r, g, b;
            getRGBFromHex(message, r, g, b);
            sendRGBToAll(r, g, b);
        }
    }

    else if (topicStr.endsWith("/debug/brightness"))
    {

        int colonPos = message.indexOf(':');
        int brightness;

        if (colonPos > 0)
        {

            int index = message.substring(0, colonPos).toInt();
            brightness = message.substring(colonPos + 1).toInt();

            if (index >= 0 && index < LUMINAIRE_NUM_LEDS)
            {
                int r = RGBpayload[index * 3 + 0];
                int g = RGBpayload[index * 3 + 1];
                int b = RGBpayload[index * 3 + 2];

                r = (r * brightness) / 255;
                g = (g * brightness) / 255;
                b = (b * brightness) / 255;

                sendRGBToPixel(r, g, b, index);
            }
        }
        else
        {

            brightness = message.toInt();
            for (int i = 0; i < LUMINAIRE_NUM_LEDS; i++)
            {
                int r = RGBpayload[i * 3 + 0];
                int g = RGBpayload[i * 3 + 1];
                int b = RGBpayload[i * 3 + 2];

                r = (r * brightness) / 255;
                g = (g * brightness) / 255;
                b = (b * brightness) / 255;

                RGBpayload[i * 3 + 0] = (byte)r;
                RGBpayload[i * 3 + 1] = (byte)g;
                RGBpayload[i * 3 + 2] = (byte)b;
            }

            if (mqtt && mqtt->isConnected())
            {
                mqtt->publish(mqttTopic.c_str(), RGBpayload, LUMINAIRE_PAYLOAD_SIZE, false);
            }
        }
    }

    else if (topicStr.endsWith("/debug/index"))
    {
        if (message == "clear" || message == "CLEAR")
        {
            Serial.println("[Luminaire] Clearing DEBUG mode");

            if (state == LUMI_ON)
            {
                applyModeColor();
            }
            else
            {
                clear();
            }
        }
    }
}

void LuminaireController::applyModeColor()
{
    if (!isActive)
    {
        return;
    }

    int r = 0, g = 0, b = 0;

    switch (mode)
    {
    case LUMI_MODE_TIMER:

        r = 255;
        g = 0;
        b = 0;
        break;
    case LUMI_MODE_WEATHER:

        r = 0;
        g = 255;
        b = 0;
        break;
    case LUMI_MODE_IDLE:
        // IDLE 模式不立即发送颜色，由呼吸灯循环处理
        return;
    case LUMI_MODE_MUSIC:

        r = 255;
        g = 255;
        b = 255;
        break;
    }

    sendRGBToAll(r, g, b);
}

void LuminaireController::updateBreathingEffect()
{
    unsigned long now = millis();

    // 每20ms更新一次亮度
    if (now - lastBreathUpdate > 20)
    {
        breathBrightness += breathDirection * 2;

        if (breathBrightness >= 255)
        {
            breathBrightness = 255;
            breathDirection = -1;
        }
        else if (breathBrightness <= 0)
        {
            breathBrightness = 0;
            breathDirection = 1;
        }

        // 应用呼吸效果到IDLE颜色
        int r = (idleColor >> 16) & 0xFF;
        int g = (idleColor >> 8) & 0xFF;
        int b = idleColor & 0xFF;

        // 根据呼吸亮度调整RGB值
        r = (r * breathBrightness) / 255;
        g = (g * breathBrightness) / 255;
        b = (b * breathBrightness) / 255;

        sendRGBToAll(r, g, b);

        lastBreathUpdate = now;
    }
}

void LuminaireController::updateMusicSpectrum()
{
    // 提前检查 MQTT 连接，避免每个像素都检查
    if (!mqtt || !mqtt->isConnected())
    {
        static unsigned long lastWarning = 0;
        // 每 5 秒最多警告一次，避免日志洪水
        if (millis() - lastWarning > 5000)
        {
            Serial.println("[Luminaire] ✗ MQTT not connected, skipping spectrum update");
            lastWarning = millis();
        }
        return;
    }

    // 72 灯布局：12 列（频段）× 6 行（高度）
    // LED 索引映射（每列倒序）：
    // 列 0: 5,4,3,2,1,0 (底→顶)
    // 列 1: 11,10,9,8,7,6
    // 列 2: 17,16,15,14,13,12
    // ...
    // 列 11: 71,70,69,68,67,66

    float bands[12];
    musicMode->getSpectrumData(bands);

    // 行颜色（从底部到顶部的渐变）
    // 底部（行 5）：绿色 → 顶部（行 0）：红色
    uint8_t rowColors[6][3] = {
        {255, 0, 0},   // 行 0（顶部）：红色（最响亮）
        {255, 128, 0}, // 行 1：橙色
        {255, 255, 0}, // 行 2：黄色
        {128, 255, 0}, // 行 3：黄绿
        {0, 255, 0},   // 行 4：绿色
        {0, 255, 128}  // 行 5（底部）：青绿（最安静）
    };

    // 调试：每 5 秒打印一次频谱数据
    static unsigned long lastDebug = 0;
    if (millis() - lastDebug > 5000)
    {
        Serial.print("[Luminaire] Spectrum: ");
        for (int i = 0; i < 12; i++)
        {
            Serial.print(bands[i], 2);
            if (i < 11)
                Serial.print(",");
        }
        Serial.println();
        lastDebug = millis();
    }

    // 填充 12 个频段（列）
    for (int col = 0; col < 12; col++)
    {
        // 计算这一列应该显示的精确高度（0.0 - 6.0）
        float exactHeight = bands[col] * 6.0;
        if (exactHeight > 6.0)
            exactHeight = 6.0;

        // 完全点亮的块数量（向下取整）
        int fullBlocks = (int)exactHeight;

        // 部分点亮的块亮度（0.0 - 1.0）
        float partialBrightness = exactHeight - fullBlocks;

        // 从底部（行 5）到顶部（行 0）填充
        for (int row = 5; row >= 0; row--)
        {
            // 计算该列该行的 LED 索引
            // 每列从底到顶：col*6+5, col*6+4, ..., col*6+0
            int ledIndex = col * 6 + row;

            // 计算当前块在这一列中的位置（0=底部，5=顶部）
            int blockPosition = 5 - row;

            int r, g, b;

            if (blockPosition < fullBlocks)
            {
                // 完全点亮：使用该行的颜色，全亮度
                r = rowColors[row][0];
                g = rowColors[row][1];
                b = rowColors[row][2];
            }
            else if (blockPosition == fullBlocks && partialBrightness > 0.05)
            {
                // 部分点亮：使用该行的颜色，按比例亮度
                r = (int)(rowColors[row][0] * partialBrightness);
                g = (int)(rowColors[row][1] * partialBrightness);
                b = (int)(rowColors[row][2] * partialBrightness);
            }
            else
            {
                // 熄灭：完全关闭
                r = 0;
                g = 0;
                b = 0;
            }

            // 直接更新 payload 数组，不发送
            RGBpayload[ledIndex * 3 + 0] = (byte)r;
            RGBpayload[ledIndex * 3 + 1] = (byte)g;
            RGBpayload[ledIndex * 3 + 2] = (byte)b;
        }
    }

    // 所有像素更新完成后，只发送一次 MQTT 消息
    mqtt->publish(mqttTopic.c_str(), RGBpayload, LUMINAIRE_PAYLOAD_SIZE, false);
}

void LuminaireController::getRGBFromHex(const String &hexColor, int &r, int &g, int &b)
{
    String color = hexColor;
    if (color.startsWith("#"))
    {
        color = color.substring(1);
    }

    if (color.length() == 6)
    {
        r = strtol(color.substring(0, 2).c_str(), NULL, 16);
        g = strtol(color.substring(2, 4).c_str(), NULL, 16);
        b = strtol(color.substring(4, 6).c_str(), NULL, 16);
    }
}

// ========================================
// 伞状LED映射工具函数
// ========================================

// 获取指定伞骨和位置的LED编号
// rib: 0-11 (12条伞骨)
// position: 0-5 (0=边缘顶部, 5=中心底部)
int LuminaireController::getUmbrellaLED(int rib, int position)
{
    if (rib < 0 || rib >= 12 || position < 0 || position >= 6)
    {
        return -1;
    }
    return rib * 6 + position; // 每条伞骨6个LED (LED 0-5)
}

// 设置单个伞骨LED的颜色
void LuminaireController::setUmbrellaPixel(int rib, int position, int r, int g, int b)
{
    int ledIndex = getUmbrellaLED(rib, position);
    if (ledIndex >= 0 && ledIndex < LUMINAIRE_NUM_LEDS)
    {
        RGBpayload[ledIndex * 3 + 0] = (byte)r;
        RGBpayload[ledIndex * 3 + 1] = (byte)g;
        RGBpayload[ledIndex * 3 + 2] = (byte)b;
    }
}

// 设置径向环（所有伞骨的同一位置）
void LuminaireController::setRadialRing(int position, int r, int g, int b)
{
    for (int rib = 0; rib < 12; rib++)
    {
        setUmbrellaPixel(rib, position, r, g, b);
    }
}

// ========================================
// 天气数据更新
// ========================================

void LuminaireController::updateWeatherData(const String &weatherJson)
{
    Serial.println("[Luminaire Weather] Parsing weather JSON...");

    // 解析JSON
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, weatherJson);

    if (error)
    {
        Serial.print("[Luminaire Weather] JSON parse error: ");
        Serial.println(error.c_str());
        return;
    }

    // 提取天气数据
    if (doc.containsKey("temp_C"))
    {
        currentTemp = doc["temp_C"].as<String>().toFloat();
    }

    if (doc.containsKey("FeelsLikeC"))
    {
        feelsLikeTemp = doc["FeelsLikeC"].as<String>().toFloat();
    }

    if (doc.containsKey("humidity"))
    {
        humidity = doc["humidity"].as<String>().toInt();
    }

    if (doc.containsKey("windspeedKmph"))
    {
        windSpeed = doc["windspeedKmph"].as<String>().toInt();
    }

    if (doc.containsKey("winddir16Point"))
    {
        windDirection = doc["winddir16Point"].as<String>();
    }

    if (doc.containsKey("visibility"))
    {
        visibility = doc["visibility"].as<String>().toInt();
    }

    if (doc.containsKey("cloudcover"))
    {
        cloudCover = doc["cloudcover"].as<String>().toInt();
    }

    if (doc.containsKey("precipMM"))
    {
        precipitation = doc["precipMM"].as<String>().toFloat();
    }

    if (doc.containsKey("weatherCode"))
    {
        weatherCode = doc["weatherCode"].as<String>();
    }

    if (doc.containsKey("weatherDesc"))
    {
        weatherDesc = doc["weatherDesc"].as<String>();
    }

    Serial.println("[Luminaire Weather] Weather data updated:");
    Serial.print("  Temperature: ");
    Serial.print(currentTemp);
    Serial.print("°C (Feels like ");
    Serial.print(feelsLikeTemp);
    Serial.println("°C)");
    Serial.print("  Humidity: ");
    Serial.print(humidity);
    Serial.println("%");
    Serial.print("  Wind: ");
    Serial.print(windSpeed);
    Serial.print(" km/h ");
    Serial.println(windDirection);
    Serial.print("  Visibility: ");
    Serial.print(visibility);
    Serial.println(" km");
    Serial.print("  Cloud cover: ");
    Serial.print(cloudCover);
    Serial.println("%");
    Serial.print("  Weather: ");
    Serial.print(weatherDesc);
    Serial.print(" (");
    Serial.print(weatherCode);
    Serial.println(")");
}

// ========================================
// 天气渲染函数（重新设计）
// ========================================

// 第一行：湿度 - 蓝色，湿度越大越亮
void LuminaireController::renderHumidity()
{
    // 湿度0-100%映射到蓝色亮度
    float humidityNorm = humidity / 100.0;

    int r = 0;
    int g = 0;
    int b = (int)(255 * humidityNorm);

    setRadialRing(0, r, g, b);
}

// 第二行：风速 - 白色追逐光点
void LuminaireController::renderWindSpeed()
{
    unsigned long now = millis();

    // 根据风速决定更新频率和光点数量
    int updateInterval;
    int numDots;
    int brightness1, brightness2, brightness3;

    if (windSpeed == 0)
    {
        // 风速为0：一个静止的白点
        numDots = 1;
        updateInterval = 999999; // 不移动
        brightness1 = 150;
        brightness2 = 0;
        brightness3 = 0;
    }
    else if (windSpeed <= 5)
    {
        // 微风：一个缓慢移动的光点
        numDots = 1;
        updateInterval = 600;
        brightness1 = 180;
        brightness2 = 0;
        brightness3 = 0;
    }
    else if (windSpeed <= 15)
    {
        // 和风：两个光点，中速
        numDots = 2;
        updateInterval = 300;
        brightness1 = 200;
        brightness2 = 120;
        brightness3 = 0;
    }
    else if (windSpeed <= 30)
    {
        // 强风：三个光点，快速，有速度感
        numDots = 3;
        updateInterval = 150;
        brightness1 = 150; // 第一个暗
        brightness2 = 255; // 第二个亮
        brightness3 = 150; // 第三个暗
    }
    else
    {
        // 疾风：三个光点，非常快，强烈速度感
        numDots = 3;
        updateInterval = 80;
        brightness1 = 120; // 第一个更暗
        brightness2 = 255; // 第二个最亮
        brightness3 = 120; // 第三个更暗
    }

    // 检查是否需要更新
    if (windSpeed > 0 && now - lastWindUpdate < updateInterval)
    {
        return; // 保持当前状态
    }

    if (windSpeed > 0)
    {
        lastWindUpdate = now;
        windAnimationOffset = (windAnimationOffset + 1) % 12;
    }

    // 清除第二行
    setRadialRing(1, 0, 0, 0);

    // 绘制光点
    if (numDots >= 1)
    {
        int rib1 = windAnimationOffset;
        setUmbrellaPixel(rib1, 1, brightness1, brightness1, brightness1);
    }
    if (numDots >= 2)
    {
        int rib2 = (windAnimationOffset + 6) % 12; // 对面位置
        setUmbrellaPixel(rib2, 1, brightness2, brightness2, brightness2);
    }
    if (numDots >= 3)
    {
        int rib3 = (windAnimationOffset + 4) % 12; // 三分之一位置
        setUmbrellaPixel(rib3, 1, brightness3, brightness3, brightness3);
    }
}

// 第三行：可见度 - 白色，5km=0%, 10km=50%, 20km=100%
void LuminaireController::renderVisibility()
{
    int brightness;

    if (visibility <= 5)
    {
        brightness = 0; // 低于5km，0%
    }
    else if (visibility >= 20)
    {
        brightness = 255; // 高于20km，100%
    }
    else
    {
        // 5-20km线性映射到0-255
        brightness = (int)((visibility - 5) / 15.0 * 255);
    }

    setRadialRing(2, brightness, brightness, brightness);
}

// 第四行：当前温度 - 白/蓝/绿/黄/红，温度越高越亮
void LuminaireController::renderTemperature()
{
    int r, g, b;

    if (currentTemp < -10)
    {
        // < -10°C: 最亮白光
        r = 255;
        g = 255;
        b = 255;
    }
    else if (currentTemp < 0)
    {
        // (-10, 0)°C: 白光，温度越低越亮
        float ratio = (currentTemp + 10) / 10.0;   // 0.0到1.0
        int brightness = (int)(255 * (1 - ratio)); // 越低越亮
        r = brightness;
        g = brightness;
        b = brightness;
    }
    else if (currentTemp < 10)
    {
        // (0, 10)°C: 蓝光，温度越高越亮
        float ratio = currentTemp / 10.0;
        r = 0;
        g = 0;
        b = (int)(255 * ratio);
    }
    else if (currentTemp < 20)
    {
        // (10, 20)°C: 绿光，温度越高越亮
        float ratio = (currentTemp - 10) / 10.0;
        r = 0;
        g = (int)(255 * ratio);
        b = 0;
    }
    else if (currentTemp < 30)
    {
        // (20, 30)°C: 黄光，温度越高越亮
        float ratio = (currentTemp - 20) / 10.0;
        r = (int)(255 * ratio);
        g = (int)(255 * ratio);
        b = 0;
    }
    else if (currentTemp < 40)
    {
        // (30, 40)°C: 红光，温度越高越亮
        float ratio = (currentTemp - 30) / 10.0;
        r = (int)(255 * ratio);
        g = 0;
        b = 0;
    }
    else
    {
        // >= 40°C: 最亮红光
        r = 255;
        g = 0;
        b = 0;
    }

    setRadialRing(3, r, g, b);
}

// 第五行：体感温度 - 闪烁的aqua或橙黄色
void LuminaireController::renderFeelsLike()
{
    unsigned long now = millis();

    float tempDiff = feelsLikeTemp - currentTemp;

    // 闪烁周期：500ms
    bool isOn = (now % 1000) < 500;

    int r, g, b;

    if (abs(tempDiff) < 0.5)
    {
        // 温差很小，不显示
        r = 0;
        g = 0;
        b = 0;
    }
    else if (tempDiff < 0)
    {
        // 体感更冷：闪烁aqua色 (青色)
        if (isOn)
        {
            r = 0;
            g = 255;
            b = 255;
        }
        else
        {
            r = 0;
            g = 0;
            b = 0;
        }
    }
    else
    {
        // 体感更热：闪烁橙黄色
        if (isOn)
        {
            r = 255;
            g = 165;
            b = 0;
        }
        else
        {
            r = 0;
            g = 0;
            b = 0;
        }
    }

    setRadialRing(4, r, g, b);
}

// 第六行：云量 - 棕色，云量越多越深
void LuminaireController::renderCloudCover()
{
    // 云量0-100%映射到棕色深度
    float cloudNorm = cloudCover / 100.0;

    // 棕色 RGB(165, 42, 42) - 深棕色
    int r = (int)(165 * cloudNorm);
    int g = (int)(42 * cloudNorm);
    int b = (int)(42 * cloudNorm);

    setRadialRing(5, r, g, b);
}

// 主天气可视化更新函数
void LuminaireController::updateWeatherVisualization()
{
    unsigned long now = millis();

    // 限制更新频率
    if (now - lastWeatherUpdate < 50)
    {
        return;
    }
    lastWeatherUpdate = now;

    // 调试：每5秒打印一次天气数据
    static unsigned long lastDebugPrint = 0;
    if (now - lastDebugPrint > 5000)
    {
        Serial.println("\n[Weather Viz] Current data:");
        Serial.print("  Temp: ");
        Serial.print(currentTemp);
        Serial.println("°C");
        Serial.print("  Feels: ");
        Serial.print(feelsLikeTemp);
        Serial.println("°C");
        Serial.print("  Humidity: ");
        Serial.print(humidity);
        Serial.println("%");
        Serial.print("  Wind: ");
        Serial.print(windSpeed);
        Serial.println(" km/h");
        Serial.print("  Visibility: ");
        Serial.print(visibility);
        Serial.println(" km");
        Serial.print("  Clouds: ");
        Serial.print(cloudCover);
        Serial.println("%");
        lastDebugPrint = now;
    }

    // 清空payload
    memset(RGBpayload, 0, LUMINAIRE_PAYLOAD_SIZE);

    // 按层渲染（新的6行设计）
    renderHumidity();    // 第一行：湿度 (位置0)
    renderWindSpeed();   // 第二行：风速 (位置1)
    renderVisibility();  // 第三行：可见度 (位置2)
    renderTemperature(); // 第四行：当前温度 (位置3)
    renderFeelsLike();   // 第五行：体感温度 (位置4)
    renderCloudCover();  // 第六行：云量 (位置5)

    // 发送到MQTT
    if (mqtt && mqtt->isConnected())
    {
        mqtt->publish(mqttTopic.c_str(), RGBpayload, LUMINAIRE_PAYLOAD_SIZE, false);
    }
}
