#include "mqtt_manager.h"

// Constructor
MQTTManager::MQTTManager()
{
    wifiClient = new WiFiClient();
    mqttClient = new PubSubClient(*wifiClient);
    lastReconnectAttempt = 0;
    messageCallback = nullptr;
}

// Destructor
MQTTManager::~MQTTManager()
{
    if (mqttClient->connected())
    {
        mqttClient->disconnect();
    }
    delete mqttClient;
    delete wifiClient;
}

// Generate unique client ID from MAC address
String MQTTManager::generateClientID()
{
    byte mac[6];
    WiFi.macAddress(mac);

    String macStr = "";
    for (int i = 0; i < 6; i++)
    {
        if (mac[i] < 0x10)
            macStr += "0";
        macStr += String(mac[i], HEX);
    }
    macStr.toUpperCase();

    return String(MQTT_CLIENT_ID_PREFIX) + macStr;
}

// Initialize MQTT client
void MQTTManager::begin()
{
    // Generate unique client ID
    clientID = generateClientID();

    Serial.println("\n========================================");
    Serial.println("[MQTT] Initializing MQTT Manager");
    Serial.println("========================================");
    Serial.print("[MQTT] Broker: ");
    Serial.print(MQTT_SERVER);
    Serial.print(":");
    Serial.println(MQTT_PORT);
    Serial.print("[MQTT] Client ID: ");
    Serial.println(clientID);
    Serial.print("[MQTT] Username: ");
    Serial.println(MQTT_USERNAME);
    Serial.println("========================================\n");

    // Configure MQTT client
    mqttClient->setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient->setKeepAlive(MQTT_KEEPALIVE);

    // Set buffer size (default is 256 bytes, increase if needed)
    mqttClient->setBufferSize(512);
}

// Connect to MQTT broker
bool MQTTManager::connect()
{
    Serial.print("[MQTT] Connecting to broker...");

    // Attempt to connect with username and password
    bool connected = mqttClient->connect(
        clientID.c_str(),
        MQTT_USERNAME,
        MQTT_PASSWORD,
        TOPIC_STATUS, // Will topic
        0,            // Will QoS
        true,         // Will retain
        "offline"     // Will message
    );

    if (connected)
    {
        Serial.println(" ✓ Connected");

        // Publish online status
        publishStatus("online");

        // V2.0: Subscribe to new topics
        Serial.println("[MQTT] Subscribing to topics...");

        subscribe(TOPIC_STATUS);
        Serial.print("[MQTT] ✓ Subscribed to: ");
        Serial.println(TOPIC_STATUS);

        subscribe(TOPIC_MODE);
        Serial.print("[MQTT] ✓ Subscribed to: ");
        Serial.println(TOPIC_MODE);

        // DEBUG topics
        subscribe(TOPIC_DEBUG_COLOR);
        Serial.print("[MQTT] ✓ Subscribed to: ");
        Serial.println(TOPIC_DEBUG_COLOR);

        subscribe(TOPIC_DEBUG_BRIGHTNESS);
        Serial.print("[MQTT] ✓ Subscribed to: ");
        Serial.println(TOPIC_DEBUG_BRIGHTNESS);

        subscribe(TOPIC_DEBUG_INDEX);
        Serial.print("[MQTT] ✓ Subscribed to: ");
        Serial.println(TOPIC_DEBUG_INDEX);

        Serial.println("[MQTT] ========================================");
        Serial.println("[MQTT] MQTT connection established successfully");
        Serial.println("[MQTT] ========================================\n");
    }
    else
    {
        Serial.print(" ✗ Failed, rc=");
        Serial.println(mqttClient->state());

        // Print error description
        switch (mqttClient->state())
        {
        case -4:
            Serial.println("[MQTT] Error: Connection timeout");
            break;
        case -3:
            Serial.println("[MQTT] Error: Connection lost");
            break;
        case -2:
            Serial.println("[MQTT] Error: Connect failed");
            break;
        case -1:
            Serial.println("[MQTT] Error: Disconnected");
            break;
        case 1:
            Serial.println("[MQTT] Error: Bad protocol");
            break;
        case 2:
            Serial.println("[MQTT] Error: Bad client ID");
            break;
        case 3:
            Serial.println("[MQTT] Error: Server unavailable");
            break;
        case 4:
            Serial.println("[MQTT] Error: Bad credentials");
            break;
        case 5:
            Serial.println("[MQTT] Error: Not authorized");
            break;
        default:
            Serial.println("[MQTT] Error: Unknown error");
            break;
        }
    }

    return connected;
}

// Reconnect to MQTT broker (non-blocking)
bool MQTTManager::reconnect()
{
    unsigned long now = millis();

    // Only attempt reconnect every RECONNECT_INTERVAL milliseconds
    if (now - lastReconnectAttempt < RECONNECT_INTERVAL)
    {
        return false;
    }

    lastReconnectAttempt = now;

    Serial.println("[MQTT] Attempting to reconnect...");
    return connect();
}

// Maintain MQTT connection (call in loop())
void MQTTManager::loop()
{
    if (!mqttClient->connected())
    {
        reconnect();
    }
    else
    {
        mqttClient->loop();
    }
}

// Check if connected
bool MQTTManager::isConnected()
{
    return mqttClient->connected();
}

// Publish message to topic
bool MQTTManager::publish(const char *topic, const char *payload)
{
    return publish(topic, payload, false);
}

bool MQTTManager::publish(const char *topic, const char *payload, bool retained)
{
    if (!mqttClient->connected())
    {
        Serial.println("[MQTT] ✗ Not connected, cannot publish");
        return false;
    }

    bool success = mqttClient->publish(topic, payload, retained);

    if (success)
    {
        Serial.print("[MQTT] ✓ Published to ");
        Serial.print(topic);
        Serial.print(": ");
        Serial.println(payload);
    }
    else
    {
        Serial.print("[MQTT] ✗ Failed to publish to ");
        Serial.println(topic);
    }

    return success;
}

// Subscribe to topic
bool MQTTManager::subscribe(const char *topic)
{
    if (!mqttClient->connected())
    {
        Serial.println("[MQTT] ✗ Not connected, cannot subscribe");
        return false;
    }

    bool success = mqttClient->subscribe(topic);

    if (success)
    {
        Serial.print("[MQTT] ✓ Subscribed to: ");
        Serial.println(topic);
    }
    else
    {
        Serial.print("[MQTT] ✗ Failed to subscribe to: ");
        Serial.println(topic);
    }

    return success;
}

// Unsubscribe from topic
bool MQTTManager::unsubscribe(const char *topic)
{
    if (!mqttClient->connected())
    {
        Serial.println("[MQTT] ✗ Not connected, cannot unsubscribe");
        return false;
    }

    bool success = mqttClient->unsubscribe(topic);

    if (success)
    {
        Serial.print("[MQTT] ✓ Unsubscribed from: ");
        Serial.println(topic);
    }
    else
    {
        Serial.print("[MQTT] ✗ Failed to unsubscribe from: ");
        Serial.println(topic);
    }

    return success;
}

// Set message callback function
void MQTTManager::setCallback(void (*callback)(char *, byte *, unsigned int))
{
    messageCallback = callback;
    mqttClient->setCallback(callback);
}

// Helper: Publish status message
bool MQTTManager::publishStatus(const char *status)
{
    return publish(TOPIC_STATUS, status, true); // Retained
}

// Helper: Publish mode
bool MQTTManager::publishMode(const char *mode)
{
    return publish(TOPIC_MODE, mode, true); // Retained
}

// V2.0: INFO publishers
bool MQTTManager::publishInfo_WiFi_SSID()
{
    String ssid = WiFi.SSID();
    return publish(TOPIC_INFO_WIFI_SSID, ssid.c_str(), true); // retained
}

bool MQTTManager::publishInfo_WiFi_IP()
{
    String ip = WiFi.localIP().toString();
    return publish(TOPIC_INFO_WIFI_IP, ip.c_str(), true); // retained
}

bool MQTTManager::publishInfo_WiFi_RSSI()
{
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d dBm", WiFi.RSSI());
    return publish(TOPIC_INFO_WIFI_RSSI, buffer, true); // retained
}

bool MQTTManager::publishInfo_WiFi_MAC()
{
    byte mac[6];
    WiFi.macAddress(mac);
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
    return publish(TOPIC_INFO_WIFI_MAC, buffer, true); // retained
}

bool MQTTManager::publishInfo_Lighter_Number(int count)
{
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%d", count);
    return publish(TOPIC_INFO_LIGHTER_NUMBER, buffer, true); // retained
}

bool MQTTManager::publishInfo_Lighter_Pin(int pin)
{
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%d", pin);
    return publish(TOPIC_INFO_LIGHTER_PIN, buffer, true); // retained
}

bool MQTTManager::publishInfo_System_Version(const char *version)
{
    return publish(TOPIC_INFO_SYSTEM_VERSION, version, true); // retained
}

bool MQTTManager::publishInfo_System_Uptime()
{
    unsigned long uptime = millis() / 1000; // 秒
    char buffer[32];
    unsigned long hours = uptime / 3600;
    unsigned long minutes = (uptime % 3600) / 60;
    unsigned long seconds = uptime % 60;
    snprintf(buffer, sizeof(buffer), "%luh %lum %lus", hours, minutes, seconds);
    return publish(TOPIC_INFO_SYSTEM_UPTIME, buffer, true); // retained
}

bool MQTTManager::publishInfo_Location_City(const char *city)
{
    return publish(TOPIC_INFO_LOCATION_CITY, city, true); // retained
}

// V2.0: 批量发布所有INFO信息
void MQTTManager::publishAllInfo(int lighterNumber, int lighterPin, const char *version, const char *city)
{
    Serial.println("[MQTT] Publishing all INFO topics...");

    publishInfo_WiFi_SSID();
    delay(50);
    publishInfo_WiFi_IP();
    delay(50);
    publishInfo_WiFi_RSSI();
    delay(50);
    publishInfo_WiFi_MAC();
    delay(50);

    publishInfo_Lighter_Number(lighterNumber);
    delay(50);
    publishInfo_Lighter_Pin(lighterPin);
    delay(50);

    publishInfo_System_Version(version);
    delay(50);
    publishInfo_System_Uptime();
    delay(50);

    publishInfo_Location_City(city);

    Serial.println("[MQTT] ✓ All INFO published");
}
