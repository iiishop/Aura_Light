#include "light_controller.h"
#include "music_mode.h"
#include "audio_analyzer.h"

LightController::LightController()
{
    mqtt = nullptr;
    musicMode = nullptr;
    audioAnalyzer = nullptr;
    strip = nullptr;
    numPixels = DEFAULT_NUM_PIXELS;
    state = LIGHT_OFF;
    mode = MODE_IDLE;

    debugData = nullptr;
    debugModeActive = false;
    debugSelectedIndex = -1;

    lastBreathUpdate = 0;
    breathDirection = 1;
    breathBrightness = 0;

    suppressMqttFeedback = false;
}

LightController::~LightController()
{

    if (strip != nullptr)
    {
        delete strip;
        strip = nullptr;
    }

    if (debugData != nullptr)
    {
        delete[] debugData;
        debugData = nullptr;
    }
}

void LightController::begin(MQTTManager *mqttManager, int pixelCount)
{
    mqtt = mqttManager;
    numPixels = constrain(pixelCount, 1, MAX_NUM_PIXELS);

    Serial.println("[LightController] Initializing...");

    if (strip != nullptr)
    {
        delete strip;
    }
    strip = new Adafruit_NeoPixel(numPixels, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
    strip->begin();
    strip->clear();
    strip->show();

    if (debugData != nullptr)
    {
        delete[] debugData;
    }
    debugData = new PixelDebugData[numPixels];

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

void LightController::setMusicMode(MusicMode *music, AudioAnalyzer *audio)
{
    musicMode = music;
    audioAnalyzer = audio;
    Serial.println("[LightController] Music mode and audio analyzer configured");
}

void LightController::setActive(bool active)
{
    if (!active)
    {

        strip->clear();
        strip->show();
        Serial.println("[LightController] Controller deactivated");
    }
    else
    {

        updateLEDs();
        Serial.println("[LightController] Controller activated");
    }
}

void LightController::setNumPixels(int count)
{
    count = constrain(count, 1, MAX_NUM_PIXELS);
    if (count != numPixels)
    {
        numPixels = count;

        delete strip;
        strip = new Adafruit_NeoPixel(numPixels, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
        strip->begin();

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

void LightController::loop()
{
    if (state != LIGHT_ON)
    {
        return; // 灯关闭时不更新
    }

    if (mode == MODE_IDLE)
    {
        updateBreathingEffect();
    }
    else if (mode == MODE_MUSIC)
    {
        // Music 模式：持续更新 VU 表
        updateLEDs();
    }
}

void LightController::handleMQTTMessage(char *topic, byte *payload, unsigned int length)
{

    String message = "";
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }

    Serial.print("[LightController] Received [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.println(message);

    String topicStr = String(topic);

    if (topicStr.endsWith("/status"))
    {
        if (message == "on" || message == "ON" || message == "1")
        {

            Serial.println("[LightController] Setting state to ON");
            state = LIGHT_ON;
            updateLEDs();
        }
        else if (message == "off" || message == "OFF" || message == "0")
        {

            Serial.println("[LightController] Setting state to OFF");
            state = LIGHT_OFF;
            updateLEDs();
        }
    }

    else if (topicStr.endsWith("/mode"))
    {

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
        else if (message == "music")
        {
            mode = MODE_MUSIC;
        }

        const char *modeNames[] = {"timer", "weather", "idle", "music"};
        Serial.print("[LightController] Setting mode to: ");
        Serial.println(modeNames[mode]);

        applyModeColor();
        if (state == LIGHT_ON)
        {
            updateLEDs();
        }
    }

    else if (topicStr.endsWith("/debug/color"))
    {

        int colonPos = message.indexOf(':');
        if (colonPos > 0)
        {

            int index = message.substring(0, colonPos).toInt();
            String colorStr = message.substring(colonPos + 1);
            debugSetColor(index, colorStr);
        }
        else
        {

            for (int i = 0; i < numPixels; i++)
            {
                debugSetColor(i, message);
            }
        }
    }

    else if (topicStr.endsWith("/debug/brightness"))
    {

        int colonPos = message.indexOf(':');
        if (colonPos > 0)
        {

            int index = message.substring(0, colonPos).toInt();
            int brightness = message.substring(colonPos + 1).toInt();
            debugSetBrightness(index, brightness);
        }
        else
        {

            int brightness = message.toInt();
            for (int i = 0; i < numPixels; i++)
            {
                debugSetBrightness(i, brightness);
            }
        }
    }

    else if (topicStr.endsWith("/debug/index"))
    {

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

void LightController::turnOn()
{
    Serial.println("[LightController] Turning ON");
    state = LIGHT_ON;
    updateLEDs();

    if (mqtt && !suppressMqttFeedback)
    {
        mqtt->publishStatus("on");
    }
}

void LightController::turnOff()
{
    Serial.println("[LightController] Turning OFF");
    state = LIGHT_OFF;
    updateLEDs();

    if (mqtt && !suppressMqttFeedback)
    {
        mqtt->publishStatus("off");
    }
}

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

        breathBrightness = 0;
        breathDirection = 1;
        lastBreathUpdate = millis();
    }
    else if (modeName == "music")
    {
        mode = MODE_MUSIC;
    }
    else
    {
        Serial.print("[LightController] ⚠ Unknown mode: ");
        Serial.println(modeName);
        suppressMqttFeedback = false;
        return;
    }

    const char *modeNames[] = {"timer", "weather", "idle", "music"};
    Serial.print("[LightController] Mode set to: ");
    Serial.println(modeNames[mode]);

    applyModeColor();

    if (state == LIGHT_ON)
    {
        updateLEDs();
    }

    if (mqtt && !suppressMqttFeedback)
    {
        mqtt->publishMode(modeNames[mode]);
    }
}

void LightController::applyModeColor()
{
    uint32_t modeColor;

    switch (mode)
    {
    case MODE_TIMER:
        modeColor = 0xFF0000;
        Serial.println("[LightController] Mode color: RED");
        break;
    case MODE_WEATHER:
        modeColor = 0x00FF00;
        Serial.println("[LightController] Mode color: GREEN");
        break;
    case MODE_IDLE:
        modeColor = 0x0000FF;
        Serial.println("[LightController] Mode color: BLUE");
        break;
    case MODE_MUSIC:
        modeColor = 0xFFFFFF;
        Serial.println("[LightController] Mode color: WHITE (Music Mode)");
        break;
    }

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

void LightController::setPixel(int index, uint32_t pixelColor, int pixelBrightness)
{
    if (strip == nullptr || index < 0 || index >= numPixels)
        return;

    uint8_t r = (pixelColor >> 16) & 0xFF;
    uint8_t g = (pixelColor >> 8) & 0xFF;
    uint8_t b = pixelColor & 0xFF;

    r = (r * pixelBrightness) / 255;
    g = (g * pixelBrightness) / 255;
    b = (b * pixelBrightness) / 255;

    strip->setPixelColor(index, strip->Color(r, g, b));
}

void LightController::debugSetColor(int index, String hexColor)
{
    if (index < 0 || index >= numPixels)
        return;

    uint32_t color = hexToColor(hexColor);

    debugData[index].color = color;
    debugData[index].isOverridden = true;
    debugModeActive = true;

    Serial.print("[LightController] DEBUG: Pixel ");
    Serial.print(index);
    Serial.print(" color set to ");
    Serial.println(hexColor);

    if (state == LIGHT_ON)
    {
        updateLEDs();
    }
}

void LightController::debugSetBrightness(int index, int brightness)
{
    if (index < 0 || index >= numPixels)
        return;

    brightness = constrain(brightness, 0, 255);

    debugData[index].brightness = brightness;
    debugData[index].isOverridden = true;
    debugModeActive = true;

    Serial.print("[LightController] DEBUG: Pixel ");
    Serial.print(index);
    Serial.print(" brightness set to ");
    Serial.println(brightness);

    if (state == LIGHT_ON)
    {
        updateLEDs();
    }
}

void LightController::debugSetIndex(int index)
{
    debugSelectedIndex = index;
    Serial.print("[LightController] DEBUG: Selected index ");
    Serial.println(index);
}

void LightController::clearDebugMode()
{
    Serial.println("[LightController] DEBUG: Clearing debug mode");

    for (int i = 0; i < numPixels; i++)
    {
        debugData[i].isOverridden = false;
    }
    debugModeActive = false;
    debugSelectedIndex = -1;

    applyModeColor();

    if (state == LIGHT_ON)
    {
        updateLEDs();
    }
}

uint32_t LightController::hexToColor(String hexColor)
{

    if (hexColor.startsWith("#"))
    {
        hexColor = hexColor.substring(1);
    }

    long number = strtol(hexColor.c_str(), NULL, 16);
    return (uint32_t)number;
}

void LightController::updateBreathingEffect()
{
    unsigned long now = millis();

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

        updateLEDs();

        lastBreathUpdate = now;
    }
}

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

    // Music 模式：使用 VU 表显示
    if (mode == MODE_MUSIC && musicMode != nullptr && audioAnalyzer != nullptr)
    {
        updateMusicVU();
        return;
    }

    // 其他模式：原有逻辑
    for (int i = 0; i < numPixels; i++)
    {
        if (debugData[i].isOverridden)
        {

            uint8_t brightness = debugData[i].brightness;

            if (mode == MODE_IDLE)
            {
                brightness = (brightness * breathBrightness) / 255;
            }

            setPixel(i, debugData[i].color, brightness);
        }
        else
        {

            uint32_t modeColor;
            switch (mode)
            {
            case MODE_TIMER:
                modeColor = 0xFF0000;
                break;
            case MODE_WEATHER:
                modeColor = 0x00FF00;
                break;
            case MODE_IDLE:
                modeColor = 0x0000FF;
                break;
            case MODE_MUSIC:
                modeColor = 0xFFFFFF;
                break;
            }

            int brightness = (mode == MODE_IDLE) ? breathBrightness : 255;
            setPixel(i, modeColor, brightness);
        }
    }

    strip->show();
}

void LightController::updateMusicVU()
{
    if (!musicMode || !audioAnalyzer)
    {
        return;
    }

    // 获取精确的音量级别（0.0 - 8.0，对应 8 个灯）
    float exactLevel = audioAnalyzer->getVolume() * 8.0;
    if (exactLevel > 8.0)
        exactLevel = 8.0;

    // 完全点亮的灯数量（向下取整）
    int fullLights = (int)exactLevel;

    // 部分点亮的灯亮度（0.0 - 1.0）
    float partialBrightness = exactLevel - fullLights;

    // 调试：每 2 秒打印一次
    static unsigned long lastDebug = 0;
    if (millis() - lastDebug > 2000)
    {
        Serial.print("[LightController] Exact Level: ");
        Serial.print(exactLevel, 2);
        Serial.print(" (Full: ");
        Serial.print(fullLights);
        Serial.print(", Partial: ");
        Serial.print(partialBrightness, 2);
        Serial.print("), Volume: ");
        Serial.print(audioAnalyzer->getVolumeDecibel(), 1);
        Serial.println(" dB");
        lastDebug = millis();
    }

    // VU 表颜色方案（从低到高）
    uint32_t vuColors[8] = {
        0x00FF00, // 0: 绿色（安静）
        0x33FF00, // 1: 黄绿
        0x66FF00, // 2: 黄绿
        0x99FF00, // 3: 黄色
        0xFFFF00, // 4: 黄色
        0xFFCC00, // 5: 橙色
        0xFF6600, // 6: 橙红
        0xFF0000  // 7: 红色（爆音）
    };

    for (int i = 0; i < numPixels && i < 8; i++)
    {
        if (debugData[i].isOverridden)
        {
            // DEBUG 模式优先
            setPixel(i, debugData[i].color, debugData[i].brightness);
        }
        else if (i < fullLights)
        {
            // 完全点亮的灯：全亮度
            setPixel(i, vuColors[i], 255);
        }
        else if (i == fullLights && partialBrightness > 0.05)
        {
            // 部分点亮的灯：使用亮度控制平滑过渡
            uint8_t brightness = (uint8_t)(partialBrightness * 255);
            setPixel(i, vuColors[i], brightness);
        }
        else
        {
            // 熄灭未达到的灯
            setPixel(i, 0x000000, 0);
        }
    }

    strip->show();
}

void LightController::publishState()
{
    if (!mqtt)
        return;

    Serial.println("[LightController] Publishing state...");

    mqtt->publishStatus(state == LIGHT_ON ? "on" : "off");

    const char *modeNames[] = {"timer", "weather", "idle", "music"};
    mqtt->publishMode(modeNames[mode]);
}
