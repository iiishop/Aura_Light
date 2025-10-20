#ifndef LUMINAIRE_CONTROLLER_H
#define LUMINAIRE_CONTROLLER_H

#include <Arduino.h>
#include "mqtt_manager.h"

// Luminaire Configuration (72 LEDs external light)
#define LUMINAIRE_NUM_LEDS 72
#define LUMINAIRE_PAYLOAD_SIZE (LUMINAIRE_NUM_LEDS * 3) // RGB for each LED

// Mode definitions
enum LuminaireMode
{
    LUMI_MODE_TIMER = 0,
    LUMI_MODE_WEATHER = 1,
    LUMI_MODE_IDLE = 2
};

// State definitions
enum LuminaireState
{
    LUMI_OFF = 0,
    LUMI_ON = 1
};

class LuminaireController
{
private:
    MQTTManager *mqtt;
    String lightId;
    String mqttTopic;
    byte RGBpayload[LUMINAIRE_PAYLOAD_SIZE];

    bool isActive;
    LuminaireState state;
    LuminaireMode mode;

    // Mode colors (RGB)
    void applyModeColor();
    void getRGBFromHex(const String &hexColor, int &r, int &g, int &b);

public:
    LuminaireController();
    void begin(MQTTManager *mqttManager, const String &id);
    void setActive(bool active);
    bool getActive() const { return isActive; }

    // MQTT message handler
    void handleMQTTMessage(char *topic, byte *payload, unsigned int length);

    // Send RGB to single pixel (0-71)
    void sendRGBToPixel(int r, int g, int b, int pixel);

    // Send RGB to all pixels
    void sendRGBToAll(int r, int g, int b);

    // Clear all pixels (turn off)
    void clear();

    // Get number of LEDs
    int getNumLEDs() const { return LUMINAIRE_NUM_LEDS; }
};

#endif
