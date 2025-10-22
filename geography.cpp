#include "geography.h"
#include "city_mapping.h" 
#include <WiFiNINA.h>

String getCurrentCity()
{
    WiFiClient client;
    const char *host = "2025.ip138.com"; 
    const int httpPort = 80;
    String city = "Suzhou"; 
    const int maxRetries = 3;

    Serial.println("[Geography] Detecting current city via IP138...");
    Serial.print("[Geography] Connecting to ");
    Serial.println(host);

    
    bool connected = false;
    for (int attempt = 1; attempt <= maxRetries; attempt++)
    {
        Serial.print("[Geography] Connection attempt ");
        Serial.print(attempt);
        Serial.print("/");
        Serial.println(maxRetries);

        if (client.connect(host, httpPort))
        {
            connected = true;
            Serial.println("[Geography] ✓ Connected successfully");
            break;
        }

        Serial.print("[Geography] ✗ Attempt ");
        Serial.print(attempt);
        Serial.println(" failed");

        if (attempt < maxRetries)
        {
            Serial.println("[Geography] Retrying in 2 seconds...");
            delay(2000);
        }
    }

    if (!connected)
    {
        Serial.println("[Geography] ✗ All connection attempts failed, using default city");
        return city;
    }

    
    Serial.println("[Geography] Sending request...");
    client.print(String("GET / HTTP/1.1\r\n") +
                 "Host: " + host + "\r\n" +
                 "User-Agent: Mozilla/5.0\r\n" +
                 "Accept: text/html\r\n" +
                 "Connection: close\r\n\r\n");

    
    unsigned long timeout = millis();
    while (client.available() == 0)
    {
        if (millis() - timeout > 10000)
        {
            Serial.println("[Geography] ✗ Request timeout, using default city");
            client.stop();
            return city;
        }
    }

    
    Serial.println("[Geography] Reading response...");
    bool headerEnded = false;
    while (client.available() && !headerEnded)
    {
        String line = client.readStringUntil('\n');
        if (line == "\r")
        {
            headerEnded = true;
        }
    }

    
    String htmlResponse = "";
    int bytesRead = 0;
    while (client.available())
    {
        char c = client.read();
        htmlResponse += c;
        bytesRead++;

        
        if (bytesRead > 3000)
            break;
    }
    client.stop();

    Serial.print("[Geography] Response received, length: ");
    Serial.println(htmlResponse.length());

    
    
    
    

    int fromIndex = htmlResponse.indexOf("来自：");
    if (fromIndex == -1)
    {
        fromIndex = htmlResponse.indexOf("来自:");
    }
    
    if (fromIndex == -1)
    {
        fromIndex = htmlResponse.indexOf("：");
    }

    if (fromIndex != -1)
    {
        Serial.println("[Geography] Found location marker");

        
        int startIndex = fromIndex;
        if (htmlResponse.charAt(fromIndex) == '来')
        {
            startIndex += 6; 
        }
        else
        {
            startIndex += 3; 
        }

        
        int endIndex = htmlResponse.indexOf('<', startIndex);
        if (endIndex == -1)
        {
            endIndex = htmlResponse.indexOf('\n', startIndex);
        }
        if (endIndex == -1)
        {
            endIndex = startIndex + 100; 
        }

        String location = htmlResponse.substring(startIndex, endIndex);
        location.trim();

        
        int tagStart = location.indexOf('<');
        if (tagStart != -1)
        {
            location = location.substring(0, tagStart);
        }
        location.replace(" ", ""); 

        Serial.print("[Geography] Raw location string: ");
        Serial.println(location);

        
        int cityIndex = location.indexOf("市");
        bool cityFound = false;

        if (cityIndex != -1)
        {
            
            int startPos = cityIndex - 1;

            
            while (startPos > 0)
            {
                char c = location.charAt(startPos);
                if (c == '省' || c == '自' || c == '特' || c == '国')
                {
                    startPos++;
                    break;
                }
                startPos--;
            }

            
            String cityCandidate = location.substring(startPos, cityIndex);

            
            for (int i = 0; i < CITY_MAP_SIZE; i++)
            {
                if (cityCandidate.indexOf(CITY_MAP[i].ip138Name) != -1)
                {
                    city = String(CITY_MAP[i].cityName);
                    cityFound = true;
                    Serial.print("[Geography] ✓ Mapped city (with 市): ");
                    Serial.print(cityCandidate);
                    Serial.print(" → ");
                    Serial.println(city);
                    break;
                }
            }
        }

        
        if (!cityFound)
        {
            Serial.println("[Geography] Trying location string mapping...");

            for (int i = 0; i < CITY_MAP_SIZE; i++)
            {
                if (location.indexOf(CITY_MAP[i].ip138Name) != -1)
                {
                    city = String(CITY_MAP[i].cityName);
                    cityFound = true;
                    Serial.print("[Geography] ✓ Mapped city (from location): ");
                    Serial.print(CITY_MAP[i].ip138Name);
                    Serial.print(" → ");
                    Serial.println(city);
                    break;
                }
            }
        }

        if (cityFound)
        {
            Serial.println("========================================");
            Serial.print("[Geography] ✓ City detected: ");
            Serial.println(city);
            Serial.print("[Geography] Full location: ");
            Serial.println(location);
            Serial.println("========================================\n");
        }
        else
        {
            Serial.println("[Geography] ✗ City not found in mapping table");
            Serial.print("[Geography] Location string: ");
            Serial.println(location);
            Serial.println("[Geography] Using default city");
            Serial.println("[Geography] Tip: Add this city to CITY_MAP in geography.cpp");
        }
    }
    else
    {
        Serial.println("[Geography] ✗ Location marker not found in response");
        Serial.println("[Geography] Using default city");
    }

    return city;
}
