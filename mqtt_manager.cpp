#include "mqtt_manager.h"

MQTTManager::MQTTManager()
{
    wifiClient = new WiFiClient();
    mqttClient = new PubSubClient(*wifiClient);
    lastReconnectAttempt = 0;
    messageCallback = nullptr;
}

MQTTManager::~MQTTManager()
{
    if (mqttClient->connected())
    {
        mqttClient->disconnect();
    }
    delete mqttClient;
    delete wifiClient;
}

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

void MQTTManager::begin()
{

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

    mqttClient->setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient->setKeepAlive(MQTT_KEEPALIVE);

    mqttClient->setBufferSize(512);
}

bool MQTTManager::connect()
{
    Serial.print("[MQTT] Connecting to broker...");

    bool connected = mqttClient->connect(
        clientID.c_str(),
        MQTT_USERNAME,
        MQTT_PASSWORD,
        TOPIC_STATUS,
        0,
        true,
        "offline");

    if (connected)
    {
        Serial.println(" ✓ Connected");

        publishStatus("online");

        Serial.println("[MQTT] Subscribing to topics...");

        // 订阅控制主题（不包括 /info/ 主题，避免接收自己发布的消息）
        String baseTopic = String(TOPIC_BASE);

        // 订阅状态和模式
        subscribe((baseTopic + "/status").c_str());
        subscribe((baseTopic + "/mode").c_str());
        subscribe((baseTopic + "/controller").c_str());

        // 订阅调试主题
        subscribe((baseTopic + "/debug/#").c_str());

        // 订阅 IDLE 模式颜色主题
        subscribe((baseTopic + "/idle/color").c_str());

        // 订阅音频控制主题（不包括 /info/audio/）
        subscribe((baseTopic + "/audio/volume_range").c_str());

        // 订阅天气信息主题（用于Luminaire天气可视化）
        subscribe((baseTopic + "/info/weather").c_str());

        // 订阅刷新请求
        subscribe((baseTopic + "/refresh").c_str());

        Serial.print("[MQTT] ✓ Subscribed to: ");
        Serial.println(baseTopic + "/{status,mode,controller,debug/#,idle/color,audio/volume_range,info/weather,refresh}");

        Serial.println("[MQTT] ========================================");
        Serial.println("[MQTT] MQTT connection established successfully");
        Serial.println("[MQTT] ========================================\n");
    }
    else
    {
        Serial.print(" ✗ Failed, rc=");
        Serial.println(mqttClient->state());

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

bool MQTTManager::reconnect()
{
    unsigned long now = millis();

    if (now - lastReconnectAttempt < RECONNECT_INTERVAL)
    {
        return false;
    }

    lastReconnectAttempt = now;

    Serial.println("[MQTT] Attempting to reconnect...");
    return connect();
}

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

bool MQTTManager::isConnected()
{
    return mqttClient->connected();
}

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

bool MQTTManager::publish(const char *topic, const uint8_t *payload, unsigned int length, bool retained)
{
    if (!mqttClient->connected())
    {
        Serial.println("[MQTT] ✗ Not connected, cannot publish binary data");
        return false;
    }

    bool success = mqttClient->publish(topic, payload, length, retained);

    if (success)
    {
        Serial.print("[MQTT] ✓ Published ");
        Serial.print(length);
        Serial.print(" bytes to ");
        Serial.println(topic);
    }
    else
    {
        Serial.print("[MQTT] ✗ Failed to publish binary data to ");
        Serial.println(topic);
    }

    return success;
}

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

void MQTTManager::setCallback(void (*callback)(char *, byte *, unsigned int))
{
    messageCallback = callback;
    mqttClient->setCallback(callback);
}

bool MQTTManager::publishStatus(const char *status)
{
    return publish(TOPIC_STATUS, status, true);
}

bool MQTTManager::publishMode(const char *mode)
{
    return publish(TOPIC_MODE, mode, true);
}

bool MQTTManager::publishInfo_WiFi_SSID()
{
    String ssid = WiFi.SSID();
    return publish(TOPIC_INFO_WIFI_SSID, ssid.c_str(), true);
}

bool MQTTManager::publishInfo_WiFi_IP()
{
    String ip = WiFi.localIP().toString();
    return publish(TOPIC_INFO_WIFI_IP, ip.c_str(), true);
}

bool MQTTManager::publishInfo_WiFi_RSSI()
{
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d dBm", WiFi.RSSI());
    return publish(TOPIC_INFO_WIFI_RSSI, buffer, true);
}

bool MQTTManager::publishInfo_WiFi_MAC()
{
    byte mac[6];
    WiFi.macAddress(mac);
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
    return publish(TOPIC_INFO_WIFI_MAC, buffer, true);
}

bool MQTTManager::publishInfo_Lighter_Number(int count)
{
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%d", count);
    return publish(TOPIC_INFO_LIGHTER_NUMBER, buffer, true);
}

bool MQTTManager::publishInfo_Lighter_Pin(int pin)
{
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%d", pin);
    return publish(TOPIC_INFO_LIGHTER_PIN, buffer, true);
}

bool MQTTManager::publishInfo_System_Version(const char *version)
{
    return publish(TOPIC_INFO_SYSTEM_VERSION, version, true);
}

bool MQTTManager::publishInfo_System_Uptime()
{
    unsigned long uptime = millis() / 1000;
    char buffer[32];
    unsigned long hours = uptime / 3600;
    unsigned long minutes = (uptime % 3600) / 60;
    unsigned long seconds = uptime % 60;
    snprintf(buffer, sizeof(buffer), "%luh %lum %lus", hours, minutes, seconds);
    return publish(TOPIC_INFO_SYSTEM_UPTIME, buffer, true);
}

bool MQTTManager::publishInfo_Location_City(const char *city)
{
    return publish(TOPIC_INFO_LOCATION_CITY, city, true);
}

bool MQTTManager::publishInfo(const char *subTopic, const char *payload, bool retained)
{

    String fullTopic = String(TOPIC_BASE) + "/info/" + String(subTopic);
    return publish(fullTopic.c_str(), payload, retained);
}

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
