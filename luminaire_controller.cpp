#include "luminaire_controller.h"
#include "music_mode.h"
#include "audio_analyzer.h"

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
      breathBrightness(0)
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
