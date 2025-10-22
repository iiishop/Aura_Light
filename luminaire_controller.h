#ifndef LUMINAIRE_CONTROLLER_H
#define LUMINAIRE_CONTROLLER_H

#include <Arduino.h>
#include "mqtt_manager.h"

#define LUMINAIRE_NUM_LEDS 72
#define LUMINAIRE_PAYLOAD_SIZE (LUMINAIRE_NUM_LEDS * 3) 

enum LuminaireMode
{
    LUMI_MODE_TIMER = 0,
    LUMI_MODE_WEATHER = 1,
    LUMI_MODE_IDLE = 2
};

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

    
    void applyModeColor();
    void getRGBFromHex(const String &hexColor, int &r, int &g, int &b);

public:
    LuminaireController();
    void begin(MQTTManager *mqttManager, const String &id);
    void setActive(bool active);
    bool getActive() const { return isActive; }

    
    void handleMQTTMessage(char *topic, byte *payload, unsigned int length);

    
    void sendRGBToPixel(int r, int g, int b, int pixel);

    
    void sendRGBToAll(int r, int g, int b);

    
    void clear();

    
    int getNumLEDs() const { return LUMINAIRE_NUM_LEDS; }

    
    bool isOn() const { return state == LUMI_ON; }
    LuminaireMode getMode() const { return mode; }
    const char *getModeString() const
    {
        const char *modeNames[] = {"timer", "weather", "idle"};
        return modeNames[mode];
    }
    const char *getStateString() const
    {
        return (state == LUMI_ON) ? "on" : "off";
    }
};

#endif
