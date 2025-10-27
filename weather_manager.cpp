#include "weather_manager.h"
#include "mqtt_manager.h"
#include <WiFiNINA.h>
#include <ArduinoJson.h>

WeatherManager::WeatherManager() : mqtt(nullptr), lastUpdate(0) {}

void WeatherManager::begin(MQTTManager *mqttManager, const String &cityName)
{
    mqtt = mqttManager;
    city = cityName;
    lastUpdate = 0; // 设为0表示还未获取天气

    Serial.println("[Weather] Weather Manager initialized");
    Serial.print("[Weather] City: ");
    Serial.println(city);
}

// 检查是否已接收到天气数据
bool WeatherManager::hasReceivedWeather() const
{
    return lastUpdate > 0; // lastUpdate > 0 表示已经获取过天气
}

void WeatherManager::loop()
{
    unsigned long currentTime = millis();

    if (lastUpdate == 0 || (currentTime - lastUpdate >= UPDATE_INTERVAL))
    {
        if (mqtt && mqtt->isConnected())
        {
            fetchAndPublishWeather();
            // lastUpdate 已在 fetchAndPublishWeather() 内部设置
        }
    }
}

void WeatherManager::fetchAndPublishWeather()
{
    Serial.println("\n========================================");
    Serial.println("[Weather] Fetching weather data...");
    Serial.print("[Weather] City: ");
    Serial.println(city);
    Serial.println("========================================");

    const int MAX_RETRIES = 3;            // 最多重试3次
    const int CONNECTION_TIMEOUT = 15000; // 连接超时15秒
    const int RESPONSE_TIMEOUT = 12000;   // 响应超时12秒

    for (int attempt = 1; attempt <= MAX_RETRIES; attempt++)
    {
        if (attempt > 1)
        {
            Serial.print("[Weather] Retry attempt ");
            Serial.print(attempt);
            Serial.print("/");
            Serial.println(MAX_RETRIES);
            delay(2000); // 重试前等待2秒
        }

        WiFiSSLClient client;
        String host = "wttr.in";
        int port = 443;

        Serial.print("[Weather] Connecting to ");
        Serial.print(host);
        Serial.print(":");
        Serial.print(port);
        Serial.println("...");

        // 设置连接超时
        unsigned long connectStart = millis();
        client.setTimeout(CONNECTION_TIMEOUT);

        if (!client.connect(host.c_str(), port))
        {
            Serial.print("[Weather] ✗ SSL connection failed (attempt ");
            Serial.print(attempt);
            Serial.print("/");
            Serial.print(MAX_RETRIES);
            Serial.println(")");

            if (attempt >= MAX_RETRIES)
            {
                Serial.println("[Weather] ✗ All connection attempts failed");
                return;
            }
            continue; // 重试
        }

        unsigned long connectTime = millis() - connectStart;
        Serial.print("[Weather] ✓ SSL connection established (");
        Serial.print(connectTime);
        Serial.println("ms)");

        String url = "/" + city + "?format=j1";

        Serial.print("[Weather] Sending request: GET ");
        Serial.println(url);

        client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                     "Host: " + host + "\r\n" +
                     "User-Agent: ArduinoWiFi/1.0\r\n" +
                     "Connection: close\r\n\r\n");

        Serial.println("[Weather] Request sent, waiting for response...");

        unsigned long timeout = millis();
        while (client.available() == 0)
        {
            if (millis() - timeout > RESPONSE_TIMEOUT)
            {
                Serial.print("[Weather] ✗ Response timeout (");
                Serial.print(RESPONSE_TIMEOUT / 1000);
                Serial.println("s)");
                client.stop();

                if (attempt >= MAX_RETRIES)
                {
                    Serial.println("[Weather] ✗ All attempts timed out");
                    return;
                }
                break; // 跳出内层循环，继续重试
            }
            delay(100);
        }

        // 检查是否成功接收到响应
        if (client.available() == 0)
        {
            continue; // 重试下一次
        }

        Serial.println("[Weather] ✓ Response received");

        // 从这里开始处理HTTP响应
        Serial.println("[Weather] Reading HTTP headers...");
        bool headersEnded = false;
        int headerCount = 0;
        while (client.available() && !headersEnded)
        {
            String line = client.readStringUntil('\n');
            headerCount++;
            if (line == "\r")
            {
                headersEnded = true;
                Serial.print("[Weather] ✓ Headers complete (");
                Serial.print(headerCount);
                Serial.println(" lines)");
            }
        }

        Serial.println("[Weather] Searching for current_condition...");

        String searchKey = "\"current_condition\":";
        bool foundKey = false;
        String buffer = "";

        while (client.available() && !foundKey)
        {
            char c = client.read();
            buffer += c;
            if (buffer.endsWith(searchKey))
            {
                foundKey = true;
                Serial.println("[Weather] ✓ Found current_condition key");
            }

            if (buffer.length() > 100)
            {
                buffer = buffer.substring(buffer.length() - 50);
            }
        }

        if (!foundKey)
        {
            Serial.println("[Weather] ✗ current_condition not found");
            client.stop();

            if (attempt >= MAX_RETRIES)
            {
                Serial.println("[Weather] ✗ Failed to parse weather data after all retries");
                return;
            }
            continue; // 重试
        }

        Serial.println("[Weather] Reading current_condition data...");
        String conditionJson = "";
        int bracketCount = 0;
        bool inArray = false;

        while (client.available())
        {
            char c = client.read();

            if (c == '[' && !inArray)
            {
                inArray = true;
                conditionJson += c;
                continue;
            }

            if (inArray)
            {
                conditionJson += c;

                if (c == '{')
                    bracketCount++;
                if (c == '}')
                    bracketCount--;

                if (c == ']' && bracketCount == 0)
                {
                    Serial.println("[Weather] ✓ current_condition data extracted");
                    break;
                }
            }
        }

        client.stop();

        Serial.print("[Weather] Extracted ");
        Serial.print(conditionJson.length());
        Serial.println(" bytes");

        if (conditionJson.length() == 0)
        {
            Serial.println("[Weather] ✗ Empty current_condition data");

            if (attempt >= MAX_RETRIES)
            {
                return;
            }
            continue; // 重试
        }

        // 解析JSON数据
        Serial.println("[Weather] Parsing current_condition JSON...");
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, conditionJson);

        if (error)
        {
            Serial.print("[Weather] ✗ JSON parse error: ");
            Serial.println(error.c_str());

            if (attempt >= MAX_RETRIES)
            {
                return;
            }
            continue; // 重试
        }

        Serial.println("[Weather] ✓ JSON parsed successfully");

        if (doc.size() == 0)
        {
            Serial.println("[Weather] ✗ Empty JSON array");

            if (attempt >= MAX_RETRIES)
            {
                return;
            }
            continue; // 重试
        }

        JsonObject weather = doc[0];

        Serial.println("[Weather] Building weather JSON...");
        DynamicJsonDocument weatherDoc(1024);

        weatherDoc["temp_C"] = weather["temp_C"].as<String>();
        weatherDoc["temp_F"] = weather["temp_F"].as<String>();
        weatherDoc["FeelsLikeC"] = weather["FeelsLikeC"].as<String>();
        weatherDoc["FeelsLikeF"] = weather["FeelsLikeF"].as<String>();
        weatherDoc["humidity"] = weather["humidity"].as<String>();
        weatherDoc["weatherCode"] = weather["weatherCode"].as<String>();
        weatherDoc["weatherDesc"] = weather["weatherDesc"][0]["value"].as<String>();
        weatherDoc["windspeedKmph"] = weather["windspeedKmph"].as<String>();
        weatherDoc["windspeedMiles"] = weather["windspeedMiles"].as<String>();
        weatherDoc["winddir16Point"] = weather["winddir16Point"].as<String>();
        weatherDoc["precipMM"] = weather["precipMM"].as<String>();
        weatherDoc["pressure"] = weather["pressure"].as<String>();
        weatherDoc["cloudcover"] = weather["cloudcover"].as<String>();
        weatherDoc["visibility"] = weather["visibility"].as<String>();
        weatherDoc["visibilityMiles"] = weather["visibilityMiles"].as<String>();
        weatherDoc["uvIndex"] = weather["uvIndex"].as<String>();
        weatherDoc["localObsDateTime"] = weather["localObsDateTime"].as<String>();

        if (weather.containsKey("lang_zh"))
        {
            JsonArray langZh = weather["lang_zh"];
            if (langZh.size() > 0)
            {
                weatherDoc["weatherDesc_zh"] = langZh[0]["value"].as<String>();
                Serial.println("[Weather] ✓ Chinese description included");
            }
        }

        Serial.println("[Weather] ✓ Weather JSON built");

        String weatherJson;
        serializeJson(weatherDoc, weatherJson);

        Serial.print("[Weather] JSON size: ");
        Serial.print(weatherJson.length());
        Serial.println(" bytes");

        Serial.println("[Weather] Publishing to MQTT...");
        if (mqtt->publishInfo("weather", weatherJson.c_str(), true))
        {
            Serial.println("========================================");
            Serial.println("[Weather] ✓ Weather data published successfully");
            Serial.print("[Weather] Temperature: ");
            Serial.print(weather["temp_C"].as<String>());
            Serial.print("°C (Feels like ");
            Serial.print(weather["FeelsLikeC"].as<String>());
            Serial.println("°C)");
            Serial.print("[Weather] Condition: ");
            Serial.println(weather["weatherDesc"][0]["value"].as<String>());
            Serial.print("[Weather] Humidity: ");
            Serial.print(weather["humidity"].as<String>());
            Serial.println("%");
            Serial.println("========================================\n");

            // 标记天气数据已成功获取
            lastUpdate = millis();

            // 成功！退出重试循环
            return;
        }
        else
        {
            Serial.println("[Weather] ✗ Failed to publish to MQTT");

            if (attempt >= MAX_RETRIES)
            {
                return;
            }
            continue; // 重试
        }
    } // 结束for循环 (重试循环)

    // 如果到这里，说明所有重试都失败了
    Serial.println("[Weather] ✗ Failed to fetch weather after all retries");
}
