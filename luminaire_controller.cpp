#include "luminaire_controller.h"

LuminaireController::LuminaireController() : mqtt(nullptr), isActive(false), state(LUMI_OFF), mode(LUMI_MODE_IDLE)
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

    if (!mqtt || !mqtt->isConnected())
    {
        Serial.println("[Luminaire] ✗ MQTT not connected");
        return;
    }

    RGBpayload[pixel * 3 + 0] = (byte)r;
    RGBpayload[pixel * 3 + 1] = (byte)g;
    RGBpayload[pixel * 3 + 2] = (byte)b;

    if (mqtt->publish(mqttTopic.c_str(), RGBpayload, LUMINAIRE_PAYLOAD_SIZE, false))
    {
        Serial.print("[Luminaire] ✓ Sent RGB(");
        Serial.print(r);
        Serial.print(",");
        Serial.print(g);
        Serial.print(",");
        Serial.print(b);
        Serial.print(") to pixel ");
        Serial.println(pixel);
    }
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

    if (mqtt->publish(mqttTopic.c_str(), RGBpayload, LUMINAIRE_PAYLOAD_SIZE, false))
    {
        Serial.print("[Luminaire] ✓ Sent RGB(");
        Serial.print(r);
        Serial.print(",");
        Serial.print(g);
        Serial.print(",");
        Serial.print(b);
        Serial.println(") to ALL pixels");
    }
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

        r = 0;
        g = 0;
        b = 255;
        break;
    case LUMI_MODE_MUSIC:

        r = 255;
        g = 255;
        b = 255;
        break;
    }

    sendRGBToAll(r, g, b);
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
