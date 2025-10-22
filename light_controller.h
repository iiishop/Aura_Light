#ifndef LIGHT_CONTROLLER_H
#define LIGHT_CONTROLLER_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "mqtt_manager.h"

#define NEOPIXEL_PIN 0       
#define DEFAULT_NUM_PIXELS 1 
#define MAX_NUM_PIXELS 16    

enum LightState
{
    LIGHT_OFF, 
    LIGHT_ON   
};

enum LightMode
{
    MODE_TIMER,   
    MODE_WEATHER, 
    MODE_IDLE     
};

struct PixelDebugData
{
    uint32_t color;    
    int brightness;    
    bool isOverridden; 
};

class LightController
{
private:
    MQTTManager *mqtt;
    Adafruit_NeoPixel *strip; 
    int numPixels;            

    
    LightState state; 
    LightMode mode;   

    
    PixelDebugData *debugData; 
    bool debugModeActive;      
    int debugSelectedIndex;    

    
    unsigned long lastBreathUpdate;
    int breathDirection; 
    int breathBrightness;

    
    bool suppressMqttFeedback;

    
    void updateLEDs();
    void updateBreathingEffect();
    void applyModeColor(); 
    void setPixel(int index, uint32_t color, int brightness);
    void setAllPixels(uint32_t color, int brightness);
    uint32_t hexToColor(String hexColor);
    void parseDebugCommand(String message, int &index, String &value);

public:
    
    LightController();

    
    ~LightController();

    
    void begin(MQTTManager *mqttManager, int pixelCount = DEFAULT_NUM_PIXELS);

    
    void setNumPixels(int count);

    
    void setActive(bool active);

    
    void loop();

    
    void handleMQTTMessage(char *topic, byte *payload, unsigned int length);

    
    void turnOn();                   
    void turnOff();                  
    void setMode(LightMode newMode); 
    void setMode(String modeName);

    
    void debugSetColor(int index, String hexColor);     
    void debugSetBrightness(int index, int brightness); 
    void debugSetIndex(int index);                      
    void clearDebugMode();                              

    
    void publishState();

    
    bool isOn() { return state == LIGHT_ON; }
    LightMode getMode() { return mode; }
    int getNumPixels() { return numPixels; }
    const char *getModeString()
    {
        const char *modeNames[] = {"timer", "weather", "idle"};
        return modeNames[mode];
    }
    const char *getStateString()
    {
        return (state == LIGHT_ON) ? "on" : "off";
    }
};

#endif 
