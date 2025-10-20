#include "light_controller.h"

// Constructor
LightController::LightController()
{
    mqtt = nullptr;
    strip = nullptr;
    numPixels = DEFAULT_NUM_PIXELS;
    state = LIGHT_OFF;
    mode = MODE_IDLE; // 默认模式为呼吸

    debugData = nullptr; // 延迟到begin()时分配
    debugModeActive = false;
    debugSelectedIndex = -1;

    lastBreathUpdate = 0;
    breathDirection = 1;
    breathBrightness = 0;

    suppressMqttFeedback = false;
}

// Destructor
LightController::~LightController()
{
    // 清理NeoPixel对象
    if (strip != nullptr)
    {
        delete strip;
        strip = nullptr;
    }

    // 清理DEBUG数据数组
    if (debugData != nullptr)
    {
        delete[] debugData;
        debugData = nullptr;
    }
}

// Initialize with MQTT manager
void LightController::begin(MQTTManager *mqttManager, int pixelCount)
{
    mqtt = mqttManager;
    numPixels = constrain(pixelCount, 1, MAX_NUM_PIXELS);

    Serial.println("[LightController] Initializing...");

    // Initialize NeoPixel
    if (strip != nullptr)
    {
        delete strip;
    }
    strip = new Adafruit_NeoPixel(numPixels, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
    strip->begin();
    strip->clear();
    strip->show();

    // 分配DEBUG数据数组
    if (debugData != nullptr)
    {
        delete[] debugData;
    }
    debugData = new PixelDebugData[numPixels];

    // 初始化DEBUG数据
    for (int i = 0; i < numPixels; i++)
    {
        debugData[i].color = 0xFFFFFF;
        debugData[i].brightness = 255;
        debugData[i].isOverridden = false;
    }

    Serial.print("[LightController] ✓ Initialized with ");
    Serial.print(numPixels);
    Serial.print(" pixel(s) on pin ");
    Serial.println(NEOPIXEL_PIN);
}

// Set number of pixels (supports hot-swap)
void LightController::setNumPixels(int count)
{
    count = constrain(count, 1, MAX_NUM_PIXELS);
    if (count != numPixels)
    {
        numPixels = count;

        // Reinitialize strip
        delete strip;
        strip = new Adafruit_NeoPixel(numPixels, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
        strip->begin();

        // 重新分配DEBUG数据数组
        delete[] debugData;
        debugData = new PixelDebugData[numPixels];
        for (int i = 0; i < numPixels; i++)
        {
            debugData[i].color = 0xFFFFFF;
            debugData[i].brightness = 255;
            debugData[i].isOverridden = false;
        }

        Serial.print("[LightController] ✓ Pixel count updated to: ");
        Serial.println(numPixels);

        // Update display
        if (state == LIGHT_ON)
        {
            updateLEDs();
        }
        else
        {
            strip->clear();
            strip->show();
        }
    }
}

// Main loop
void LightController::loop()
{
    // V2.0: 只在IDLE模式下更新呼吸效果
    if (mode == MODE_IDLE && state == LIGHT_ON)
    {
        updateBreathingEffect();
    }
}

// Handle MQTT messages
void LightController::handleMQTTMessage(char *topic, byte *payload, unsigned int length)
{
    // Convert payload to string
    String message = "";
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }

    Serial.print("[LightController] Received [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.println(message);

    // Parse topic
    String topicStr = String(topic);

    // V2.0: STATUS - on/off底层控制
    if (topicStr.endsWith("/status"))
    {
        if (message == "on" || message == "ON" || message == "1")
        {
            // 只更新状态，不发布 (Dashboard已经知道了)
            Serial.println("[LightController] Setting state to ON");
            state = LIGHT_ON;
            updateLEDs();
        }
        else if (message == "off" || message == "OFF" || message == "0")
        {
            // 只更新状态，不发布 (Dashboard已经知道了)
            Serial.println("[LightController] Setting state to OFF");
            state = LIGHT_OFF;
            updateLEDs();
        }
    }

    // V2.0: MODE - timer/weather/idle
    else if (topicStr.endsWith("/mode"))
    {
        // 只更新模式，不发布 (Dashboard已经知道了)
        message.toLowerCase();
        if (message == "timer")
        {
            mode = MODE_TIMER;
        }
        else if (message == "weather")
        {
            mode = MODE_WEATHER;
        }
        else if (message == "idle")
        {
            mode = MODE_IDLE;
            breathBrightness = 0;
            breathDirection = 1;
            lastBreathUpdate = millis();
        }

        const char *modeNames[] = {"timer", "weather", "idle"};
        Serial.print("[LightController] Setting mode to: ");
        Serial.println(modeNames[mode]);

        applyModeColor();
        if (state == LIGHT_ON)
        {
            updateLEDs();
        }
    }

    // V2.0: DEBUG - debug/color
    else if (topicStr.endsWith("/debug/color"))
    {
        // 格式: "0:#FF0000" 或 "#FF0000" (应用到所有)
        int colonPos = message.indexOf(':');
        if (colonPos > 0)
        {
            // 指定索引
            int index = message.substring(0, colonPos).toInt();
            String colorStr = message.substring(colonPos + 1);
            debugSetColor(index, colorStr);
        }
        else
        {
            // 应用到所有灯珠
            for (int i = 0; i < numPixels; i++)
            {
                debugSetColor(i, message);
            }
        }
    }

    // V2.0: DEBUG - debug/brightness
    else if (topicStr.endsWith("/debug/brightness"))
    {
        // 格式: "0:128" 或 "128" (应用到所有)
        int colonPos = message.indexOf(':');
        if (colonPos > 0)
        {
            // 指定索引
            int index = message.substring(0, colonPos).toInt();
            int brightness = message.substring(colonPos + 1).toInt();
            debugSetBrightness(index, brightness);
        }
        else
        {
            // 应用到所有灯珠
            int brightness = message.toInt();
            for (int i = 0; i < numPixels; i++)
            {
                debugSetBrightness(i, brightness);
            }
        }
    }

    // V2.0: DEBUG - debug/index
    else if (topicStr.endsWith("/debug/index"))
    {
        // 选择要调试的灯珠索引
        if (message == "clear" || message == "CLEAR")
        {
            clearDebugMode();
        }
        else
        {
            debugSetIndex(message.toInt());
        }
    }
}

// Turn on light
void LightController::turnOn()
{
    Serial.println("[LightController] Turning ON");
    state = LIGHT_ON;
    updateLEDs();

    // 只在非MQTT消息触发时发布 (防止循环)
    if (mqtt && !suppressMqttFeedback)
    {
        mqtt->publishStatus("on");
    }
}

// Turn off light
void LightController::turnOff()
{
    Serial.println("[LightController] Turning OFF");
    state = LIGHT_OFF;
    updateLEDs();

    // 只在非MQTT消息触发时发布 (防止循环)
    if (mqtt && !suppressMqttFeedback)
    {
        mqtt->publishStatus("off");
    }
}

// V2.0: Set mode
void LightController::setMode(String modeName)
{
    modeName.toLowerCase();

    LightMode oldMode = mode;

    if (modeName == "timer")
    {
        mode = MODE_TIMER;
    }
    else if (modeName == "weather")
    {
        mode = MODE_WEATHER;
    }
    else if (modeName == "idle")
    {
        mode = MODE_IDLE;
        // 重置呼吸效果
        breathBrightness = 0;
        breathDirection = 1;
        lastBreathUpdate = millis();
    }
    else
    {
        Serial.print("[LightController] ⚠ Unknown mode: ");
        Serial.println(modeName);
        suppressMqttFeedback = false;
        return;
    }

    const char *modeNames[] = {"timer", "weather", "idle"};
    Serial.print("[LightController] Mode set to: ");
    Serial.println(modeNames[mode]);

    // 应用模式颜色
    applyModeColor();

    // 更新显示
    if (state == LIGHT_ON)
    {
        updateLEDs();
    }

    // 发布模式变化 (只在非MQTT消息触发时)
    if (mqtt && !suppressMqttFeedback)
    {
        mqtt->publishMode(modeNames[mode]);
    }
}

// V2.0: 应用模式颜色
void LightController::applyModeColor()
{
    uint32_t modeColor;

    switch (mode)
    {
    case MODE_TIMER:
        modeColor = 0xFF0000; // 红色
        Serial.println("[LightController] Mode color: RED");
        break;
    case MODE_WEATHER:
        modeColor = 0x00FF00; // 绿色
        Serial.println("[LightController] Mode color: GREEN");
        break;
    case MODE_IDLE:
        modeColor = 0x0000FF; // 蓝色
        Serial.println("[LightController] Mode color: BLUE");
        break;
    }

    // 应用到所有未被DEBUG覆盖的灯珠
    if (strip != nullptr)
    {
        for (int i = 0; i < numPixels; i++)
        {
            if (!debugData[i].isOverridden)
            {
                setPixel(i, modeColor, 255);
            }
        }
    }
}

// V2.0: 设置单个灯珠
void LightController::setPixel(int index, uint32_t pixelColor, int pixelBrightness)
{
    if (strip == nullptr || index < 0 || index >= numPixels)
        return;

    // Extract RGB components
    uint8_t r = (pixelColor >> 16) & 0xFF;
    uint8_t g = (pixelColor >> 8) & 0xFF;
    uint8_t b = pixelColor & 0xFF;

    // Apply brightness
    r = (r * pixelBrightness) / 255;
    g = (g * pixelBrightness) / 255;
    b = (b * pixelBrightness) / 255;

    strip->setPixelColor(index, strip->Color(r, g, b));
}

// V2.0: DEBUG - 设置颜色
void LightController::debugSetColor(int index, String hexColor)
{
    if (index < 0 || index >= numPixels)
        return;

    // 转换颜色
    uint32_t color = hexToColor(hexColor);

    // 保存到DEBUG数据
    debugData[index].color = color;
    debugData[index].isOverridden = true;
    debugModeActive = true;

    Serial.print("[LightController] DEBUG: Pixel ");
    Serial.print(index);
    Serial.print(" color set to ");
    Serial.println(hexColor);

    // 更新显示
    if (state == LIGHT_ON)
    {
        updateLEDs();
    }
}

// V2.0: DEBUG - 设置亮度
void LightController::debugSetBrightness(int index, int brightness)
{
    if (index < 0 || index >= numPixels)
        return;

    brightness = constrain(brightness, 0, 255);

    // 保存到DEBUG数据
    debugData[index].brightness = brightness;
    debugData[index].isOverridden = true;
    debugModeActive = true;

    Serial.print("[LightController] DEBUG: Pixel ");
    Serial.print(index);
    Serial.print(" brightness set to ");
    Serial.println(brightness);

    // 更新显示
    if (state == LIGHT_ON)
    {
        updateLEDs();
    }
}

// V2.0: DEBUG - 设置索引
void LightController::debugSetIndex(int index)
{
    debugSelectedIndex = index;
    Serial.print("[LightController] DEBUG: Selected index ");
    Serial.println(index);
}

// V2.0: 清除DEBUG模式
void LightController::clearDebugMode()
{
    Serial.println("[LightController] DEBUG: Clearing debug mode");

    // 清除所有DEBUG覆盖
    for (int i = 0; i < numPixels; i++)
    {
        debugData[i].isOverridden = false;
    }
    debugModeActive = false;
    debugSelectedIndex = -1;

    // 恢复模式颜色
    applyModeColor();

    // 更新显示
    if (state == LIGHT_ON)
    {
        updateLEDs();
    }
}

// Convert hex string to color
uint32_t LightController::hexToColor(String hexColor)
{
    // Remove # if present
    if (hexColor.startsWith("#"))
    {
        hexColor = hexColor.substring(1);
    }

    // Convert to RGB
    long number = strtol(hexColor.c_str(), NULL, 16);
    return (uint32_t)number;
}

// V2.0: Update breathing effect (仅IDLE模式使用)
void LightController::updateBreathingEffect()
{
    unsigned long now = millis();

    // Update every 20ms for smooth effect
    if (now - lastBreathUpdate > 20)
    {
        breathBrightness += breathDirection * 2;

        // Reverse direction at limits
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

        // Update LEDs with breathing brightness
        updateLEDs();

        lastBreathUpdate = now;
    }
}

// V2.0: Update LEDs
void LightController::updateLEDs()
{
    if (strip == nullptr)
        return;

    if (state == LIGHT_OFF)
    {
        strip->clear();
        strip->show();
        return;
    }

    // V2.0: 状态=ON时，根据DEBUG模式和MODE模式渲染
    for (int i = 0; i < numPixels; i++)
    {
        if (debugData[i].isOverridden)
        {
            // DEBUG模式优先
            uint8_t brightness = debugData[i].brightness;

            // IDLE模式下的呼吸效果也应用到DEBUG灯珠
            if (mode == MODE_IDLE)
            {
                brightness = (brightness * breathBrightness) / 255;
            }

            setPixel(i, debugData[i].color, brightness);
        }
        else
        {
            // 使用MODE颜色
            uint32_t modeColor;
            switch (mode)
            {
            case MODE_TIMER:
                modeColor = 0xFF0000; // 红色
                break;
            case MODE_WEATHER:
                modeColor = 0x00FF00; // 绿色
                break;
            case MODE_IDLE:
                modeColor = 0x0000FF; // 蓝色
                break;
            }

            // IDLE模式应用呼吸效果
            int brightness = (mode == MODE_IDLE) ? breathBrightness : 255;
            setPixel(i, modeColor, brightness);
        }
    }

    strip->show();
}

// V2.0: Publish current state
void LightController::publishState()
{
    if (!mqtt)
        return;

    Serial.println("[LightController] Publishing state...");

    // Status
    mqtt->publishStatus(state == LIGHT_ON ? "on" : "off");

    // Mode
    const char *modeNames[] = {"timer", "weather", "idle"};
    mqtt->publishMode(modeNames[mode]);
}
