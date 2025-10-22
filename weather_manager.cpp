#include "weather_manager.h"
#include "mqtt_manager.h"
#include <WiFiNINA.h>
#include <ArduinoJson.h>

WeatherManager::WeatherManager() : mqtt(nullptr), lastUpdate(0) {}

void WeatherManager::begin(MQTTManager *mqttManager, const String &cityName)
{
    mqtt = mqttManager;
    city = cityName;
    lastUpdate = 0;

    Serial.println("[Weather] Weather Manager initialized");
    Serial.print("[Weather] City: ");
    Serial.println(city);
}

void WeatherManager::loop()
{
    unsigned long currentTime = millis();

    
    if (lastUpdate == 0 || (currentTime - lastUpdate >= UPDATE_INTERVAL))
    {
        if (mqtt && mqtt->isConnected())
        {
            fetchAndPublishWeather();
            lastUpdate = currentTime;
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

    WiFiSSLClient client;
    String host = "wttr.in";
    int port = 443;

    Serial.print("[Weather] Connecting to ");
    Serial.print(host);
    Serial.print(":");
    Serial.print(port);
    Serial.println("...");

    if (!client.connect(host.c_str(), port))
    {
        Serial.println("[Weather] ✗ SSL connection failed");
        return;
    }

    Serial.println("[Weather] ✓ SSL connection established");

    
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
        if (millis() - timeout > 10000)
        {
            Serial.println("[Weather] ✗ Response timeout (10s)");
            client.stop();
            return;
        }
        delay(100);
    }

    Serial.println("[Weather] ✓ Response received");

    
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
        return;
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
            else if (c == '}')
                bracketCount--;

            
            if (c == ']' && bracketCount == 0)
            {
                Serial.println("[Weather] ✓ current_condition data extracted");
                break;
            }
        }

        
        if (conditionJson.length() > 2048)
        {
            Serial.println("[Weather] ✗ Data too large");
            client.stop();
            return;
        }
    }

    client.stop();

    Serial.print("[Weather] Extracted ");
    Serial.print(conditionJson.length());
    Serial.println(" bytes");

    if (conditionJson.length() == 0)
    {
        Serial.println("[Weather] ✗ Empty condition data");
        return;
    }

    
    Serial.println("[Weather] Parsing current_condition JSON...");
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, conditionJson);

    if (error)
    {
        Serial.print("[Weather] ✗ JSON parse failed: ");
        Serial.println(error.c_str());
        return;
    }

    Serial.println("[Weather] ✓ JSON parsed successfully");

    
    if (!doc.is<JsonArray>() || doc.size() == 0)
    {
        Serial.println("[Weather] ✗ Invalid current_condition format");
        return;
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
    }
    else
    {
        Serial.println("[Weather] ✗ Failed to publish to MQTT");
    }
}
