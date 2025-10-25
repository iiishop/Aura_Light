#include "light_controller.h"

LightController::LightController()
{
    mqtt = nullptr;
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

    if (mode == MODE_IDLE && state == LIGHT_ON)
    {
        updateBreathingEffect();
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

void LightController::publishState()
{
    if (!mqtt)
        return;

    Serial.println("[LightController] Publishing state...");

    mqtt->publishStatus(state == LIGHT_ON ? "on" : "off");

    const char *modeNames[] = {"timer", "weather", "idle", "music"};
    mqtt->publishMode(modeNames[mode]);
}
