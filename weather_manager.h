#ifndef WEATHER_MANAGER_H
#define WEATHER_MANAGER_H

#include <Arduino.h>

class MQTTManager; 

class WeatherManager
{
private:
    MQTTManager *mqtt;
    String city;
    unsigned long lastUpdate;
    const unsigned long UPDATE_INTERVAL = 600000; 

public:
    WeatherManager();
    void begin(MQTTManager *mqttManager, const String &cityName);
    void loop();
    void fetchAndPublishWeather();
};

#endif
