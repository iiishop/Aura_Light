#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiNINA.h>
#include "arduino_secrets.h"

#define MQTT_CLIENT_ID_PREFIX "AuraLight_" 
#define MQTT_KEEPALIVE 60                  
#define MQTT_CLEAN_SESSION true

#define TOPIC_BASE "student/CASA0014/" MQTT_USER

#define TOPIC_STATUS TOPIC_BASE "/status" 
#define TOPIC_MODE TOPIC_BASE "/mode"     

#define TOPIC_DEBUG_COLOR TOPIC_BASE "/debug/color"           
#define TOPIC_DEBUG_BRIGHTNESS TOPIC_BASE "/debug/brightness" 
#define TOPIC_DEBUG_INDEX TOPIC_BASE "/debug/index"           

#define TOPIC_INFO_WIFI_SSID TOPIC_BASE "/info/wifi/ssid"
#define TOPIC_INFO_WIFI_IP TOPIC_BASE "/info/wifi/ip"
#define TOPIC_INFO_WIFI_RSSI TOPIC_BASE "/info/wifi/rssi"
#define TOPIC_INFO_WIFI_MAC TOPIC_BASE "/info/wifi/mac"
#define TOPIC_INFO_LIGHTER_NUMBER TOPIC_BASE "/info/lighter/number"
#define TOPIC_INFO_LIGHTER_PIN TOPIC_BASE "/info/lighter/pin"
#define TOPIC_INFO_SYSTEM_VERSION TOPIC_BASE "/info/system/version"
#define TOPIC_INFO_SYSTEM_UPTIME TOPIC_BASE "/info/system/uptime"
#define TOPIC_INFO_LOCATION_CITY TOPIC_BASE "/info/location/city"

class MQTTManager
{
private:
    WiFiClient *wifiClient;
    PubSubClient *mqttClient;
    String clientID;

    
    void (*messageCallback)(char *topic, byte *payload, unsigned int length);

    
    unsigned long lastReconnectAttempt;
    static const unsigned long RECONNECT_INTERVAL = 5000; 

    
    String generateClientID();

public:
    
    MQTTManager();

    
    ~MQTTManager();

    
    void begin();

    
    bool connect();

    
    bool reconnect();

    
    void loop();

    
    bool isConnected();

    
    bool publish(const char *topic, const char *payload);
    bool publish(const char *topic, const char *payload, bool retained);

    
    bool publish(const char *topic, const uint8_t *payload, unsigned int length, bool retained);

    
    bool subscribe(const char *topic);

    
    bool unsubscribe(const char *topic);

    
    void setCallback(void (*callback)(char *, byte *, unsigned int));

    
    bool publishStatus(const char *status); 
    bool publishMode(const char *mode);     

    
    bool publishInfo_WiFi_SSID();
    bool publishInfo_WiFi_IP();
    bool publishInfo_WiFi_RSSI();
    bool publishInfo_WiFi_MAC();
    bool publishInfo_Lighter_Number(int number);
    bool publishInfo_Lighter_Pin(int pin);
    bool publishInfo_System_Version(const char *version);
    bool publishInfo_System_Uptime();
    bool publishInfo_Location_City(const char *city);

    
    bool publishInfo(const char *subTopic, const char *payload, bool retained = true);

    
    void publishAllInfo(int numPixels, int pin, const char *version, const char *city);
};

#endif 
