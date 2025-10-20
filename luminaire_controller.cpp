#include "luminaire_controller.h"

LuminaireController::LuminaireController() : mqtt(nullptr), isActive(false), state(LUMI_OFF), mode(LUMI_MODE_IDLE)
{
    // Initialize all LEDs to off (0,0,0)
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
        // When deactivating, turn off all lights
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

    // Update the byte array
    RGBpayload[pixel * 3 + 0] = (byte)r; // Red
    RGBpayload[pixel * 3 + 1] = (byte)g; // Green
    RGBpayload[pixel * 3 + 2] = (byte)b; // Blue

    // Publish the entire array
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

    // Update all pixels in the array
    for (int pixel = 0; pixel < LUMINAIRE_NUM_LEDS; pixel++)
    {
        RGBpayload[pixel * 3 + 0] = (byte)r;
        RGBpayload[pixel * 3 + 1] = (byte)g;
        RGBpayload[pixel * 3 + 2] = (byte)b;
    }

    // Publish the entire array
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

    // Set all to black
    memset(RGBpayload, 0, LUMINAIRE_PAYLOAD_SIZE);

    mqtt->publish(mqttTopic.c_str(), RGBpayload, LUMINAIRE_PAYLOAD_SIZE, false);
    Serial.println("[Luminaire] ✓ All LEDs cleared");
}

void LuminaireController::handleMQTTMessage(char *topic, byte *payload, unsigned int length)
{
    // Convert payload to string
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

    // Handle STATUS - on/off
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

    // Handle MODE - timer/weather/idle
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

        const char *modeNames[] = {"timer", "weather", "idle"};
        Serial.print("[Luminaire] Setting mode to: ");
        Serial.println(modeNames[mode]);

        if (state == LUMI_ON)
        {
            applyModeColor();
        }
    }

    // Handle DEBUG - debug/color
    else if (topicStr.endsWith("/debug/color"))
    {
        // Format: "0:#FF0000" or "#FF0000" (apply to all)
        int colonPos = message.indexOf(':');
        if (colonPos > 0)
        {
            // Specific pixel
            int index = message.substring(0, colonPos).toInt();
            String colorStr = message.substring(colonPos + 1);
            int r, g, b;
            getRGBFromHex(colorStr, r, g, b);
            sendRGBToPixel(r, g, b, index);
        }
        else
        {
            // Apply to all pixels
            int r, g, b;
            getRGBFromHex(message, r, g, b);
            sendRGBToAll(r, g, b);
        }
    }

    // Handle DEBUG - debug/brightness
    else if (topicStr.endsWith("/debug/brightness"))
    {
        // Format: "0:128" or "128" (apply to all)
        int colonPos = message.indexOf(':');
        int brightness;

        if (colonPos > 0)
        {
            // Specific pixel
            int index = message.substring(0, colonPos).toInt();
            brightness = message.substring(colonPos + 1).toInt();

            // Get current color and adjust brightness
            if (index >= 0 && index < LUMINAIRE_NUM_LEDS)
            {
                int r = RGBpayload[index * 3 + 0];
                int g = RGBpayload[index * 3 + 1];
                int b = RGBpayload[index * 3 + 2];

                // Scale by brightness (0-255)
                r = (r * brightness) / 255;
                g = (g * brightness) / 255;
                b = (b * brightness) / 255;

                sendRGBToPixel(r, g, b, index);
            }
        }
        else
        {
            // Apply to all pixels
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

    // Handle DEBUG - debug/index (clear command)
    else if (topicStr.endsWith("/debug/index"))
    {
        if (message == "clear" || message == "CLEAR")
        {
            Serial.println("[Luminaire] Clearing DEBUG mode");
            // Reapply mode color
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
        // Red
        r = 255;
        g = 0;
        b = 0;
        break;
    case LUMI_MODE_WEATHER:
        // Green
        r = 0;
        g = 255;
        b = 0;
        break;
    case LUMI_MODE_IDLE:
        // Blue
        r = 0;
        g = 0;
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
