#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiNINA.h>
#include "arduino_secrets.h"

// MQTT Client Configuration
#define MQTT_CLIENT_ID_PREFIX "AuraLight_" // Will append MAC address
#define MQTT_KEEPALIVE 60                  // Keep alive interval (seconds)
#define MQTT_CLEAN_SESSION true

// Build topic paths dynamically using MQTT_USER from secrets
// Format: student/CASA0014/{username}/...
#define TOPIC_BASE "student/CASA0014/" MQTT_USER

// V2.0 Topic Structure
#define TOPIC_STATUS TOPIC_BASE "/status" // on/off control
#define TOPIC_MODE TOPIC_BASE "/mode"     // timer/weather/idle

// DEBUG topics
#define TOPIC_DEBUG_COLOR TOPIC_BASE "/debug/color"           // index:color
#define TOPIC_DEBUG_BRIGHTNESS TOPIC_BASE "/debug/brightness" // index:brightness
#define TOPIC_DEBUG_INDEX TOPIC_BASE "/debug/index"           // index selection

// INFO topics (publish only)
#define TOPIC_INFO_WIFI_SSID TOPIC_BASE "/info/wifi/ssid"
#define TOPIC_INFO_WIFI_IP TOPIC_BASE "/info/wifi/ip"
#define TOPIC_INFO_WIFI_RSSI TOPIC_BASE "/info/wifi/rssi"
#define TOPIC_INFO_WIFI_MAC TOPIC_BASE "/info/wifi/mac"
#define TOPIC_INFO_LIGHTER_NUMBER TOPIC_BASE "/info/lighter/number"
#define TOPIC_INFO_LIGHTER_PIN TOPIC_BASE "/info/lighter/pin"
#define TOPIC_INFO_SYSTEM_VERSION TOPIC_BASE "/info/system/version"
#define TOPIC_INFO_SYSTEM_UPTIME TOPIC_BASE "/info/system/uptime"
#define TOPIC_INFO_LOCATION_CITY TOPIC_BASE "/info/location/city"

// Class for managing MQTT connection
class MQTTManager
{
private:
    WiFiClient *wifiClient;
    PubSubClient *mqttClient;
    String clientID;

    // Callback function pointer
    void (*messageCallback)(char *topic, byte *payload, unsigned int length);

    // Connection retry settings
    unsigned long lastReconnectAttempt;
    static const unsigned long RECONNECT_INTERVAL = 5000; // 5 seconds

    // Generate unique client ID from MAC address
    String generateClientID();

public:
    // Constructor
    MQTTManager();

    // Destructor
    ~MQTTManager();

    // Initialize MQTT client
    void begin();

    // Connect to MQTT broker
    bool connect();

    // Reconnect to MQTT broker (non-blocking)
    bool reconnect();

    // Maintain MQTT connection (call in loop())
    void loop();

    // Check if connected
    bool isConnected();

    // Publish message to topic
    bool publish(const char *topic, const char *payload);
    bool publish(const char *topic, const char *payload, bool retained);

    // Publish binary data (for Luminaire)
    bool publish(const char *topic, const uint8_t *payload, unsigned int length, bool retained);

    // Subscribe to topic
    bool subscribe(const char *topic);

    // Unsubscribe from topic
    bool unsubscribe(const char *topic);

    // Set message callback function
    void setCallback(void (*callback)(char *, byte *, unsigned int));

    // V2.0 Publish helpers
    bool publishStatus(const char *status); // on/off
    bool publishMode(const char *mode);     // timer/weather/idle

    // INFO topic publishers (自动获取数据)
    bool publishInfo_WiFi_SSID();
    bool publishInfo_WiFi_IP();
    bool publishInfo_WiFi_RSSI();
    bool publishInfo_WiFi_MAC();
    bool publishInfo_Lighter_Number(int number);
    bool publishInfo_Lighter_Pin(int pin);
    bool publishInfo_System_Version(const char *version);
    bool publishInfo_System_Uptime();
    bool publishInfo_Location_City(const char *city);

    // Generic INFO publisher
    bool publishInfo(const char *subTopic, const char *payload, bool retained = true);

    // Publish all INFO at once
    void publishAllInfo(int numPixels, int pin, const char *version, const char *city);
};

#endif // MQTT_MANAGER_H
