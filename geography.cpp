#include "geography.h"
#include <WiFiNINA.h>
#include <ArduinoJson.h>

String getCurrentCity()
{
    WiFiClient client;
    const char *host = "ip-api.com";
    const int httpPort = 80;
    String city = "Suzhou"; // Default fallback city

    Serial.println("[Geography] Detecting current city...");

    // Connect to ip-api.com
    if (!client.connect(host, httpPort))
    {
        Serial.println("[Geography] ✗ Connection failed, using default city");
        return city;
    }

    // Send HTTP GET request
    String url = "/json/?fields=status,city";
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");

    // Wait for response with timeout
    unsigned long timeout = millis();
    while (client.available() == 0)
    {
        if (millis() - timeout > 5000)
        {
            Serial.println("[Geography] ✗ Request timeout, using default city");
            client.stop();
            return city;
        }
    }

    // Skip HTTP headers
    bool headerEnded = false;
    while (client.available() && !headerEnded)
    {
        String line = client.readStringUntil('\n');
        if (line == "\r")
        {
            headerEnded = true;
        }
    }

    // Read JSON response
    String jsonResponse = "";
    while (client.available())
    {
        jsonResponse += client.readString();
    }
    client.stop();

    // Parse JSON
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, jsonResponse);

    if (error)
    {
        Serial.print("[Geography] ✗ JSON parse failed: ");
        Serial.println(error.c_str());
        Serial.println("[Geography] Using default city");
        return city;
    }

    // Check status
    String status = doc["status"];
    if (status != "success")
    {
        Serial.println("[Geography] ✗ API request failed, using default city");
        return city;
    }

    // Extract city name
    city = doc["city"].as<String>();

    Serial.println("========================================");
    Serial.print("[Geography] ✓ Current city detected: ");
    Serial.println(city);
    Serial.println("========================================\n");

    return city;
}
