#include "geography.h"
#include "city_mapping.h" // City mapping table
#include <WiFiNINA.h>

String getCurrentCity()
{
    WiFiClient client;
    const char *host = "2025.ip138.com"; // ip138.com 的子域名
    const int httpPort = 80;
    String city = "Suzhou"; // Default fallback city

    Serial.println("[Geography] Detecting current city via IP138...");
    Serial.print("[Geography] Connecting to ");
    Serial.println(host);

    // Connect to ip138.com
    if (!client.connect(host, httpPort))
    {
        Serial.println("[Geography] ✗ Connection failed, using default city");
        return city;
    }

    // Send HTTP GET request
    Serial.println("[Geography] Sending request...");
    client.print(String("GET / HTTP/1.1\r\n") +
                 "Host: " + host + "\r\n" +
                 "User-Agent: Mozilla/5.0\r\n" +
                 "Accept: text/html\r\n" +
                 "Connection: close\r\n\r\n");

    // Wait for response with timeout
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

    // Skip HTTP headers
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

    // Read HTML response
    String htmlResponse = "";
    int bytesRead = 0;
    while (client.available())
    {
        char c = client.read();
        htmlResponse += c;
        bytesRead++;

        // Limit response size to prevent memory overflow
        if (bytesRead > 3000)
            break;
    }
    client.stop();

    Serial.print("[Geography] Response received, length: ");
    Serial.println(htmlResponse.length());

    // Parse city from HTML
    // IP138.com formats:
    // - 中文: "来自：中国 江苏省 苏州市" or "来自：中国 北京市"
    // - 英文: "：英国英格兰伦敦" or "：美国加利福尼亚州旧金山"

    int fromIndex = htmlResponse.indexOf("来自：");
    if (fromIndex == -1)
    {
        fromIndex = htmlResponse.indexOf("来自:");
    }
    // Also check for simplified format (just "：")
    if (fromIndex == -1)
    {
        fromIndex = htmlResponse.indexOf("：");
    }

    if (fromIndex != -1)
    {
        Serial.println("[Geography] Found location marker");

        // 找到位置信息的起始位置
        int startIndex = fromIndex;
        if (htmlResponse.charAt(fromIndex) == '来')
        {
            startIndex += 6; // "来自：" 的长度
        }
        else
        {
            startIndex += 3; // "：" 的长度 (UTF-8 encoding)
        }

        // 查找位置信息的结束位置 (通常是 < 或换行)
        int endIndex = htmlResponse.indexOf('<', startIndex);
        if (endIndex == -1)
        {
            endIndex = htmlResponse.indexOf('\n', startIndex);
        }
        if (endIndex == -1)
        {
            endIndex = startIndex + 100; // 限制长度
        }

        String location = htmlResponse.substring(startIndex, endIndex);
        location.trim();

        // 移除可能的HTML标签和多余空格
        int tagStart = location.indexOf('<');
        if (tagStart != -1)
        {
            location = location.substring(0, tagStart);
        }
        location.replace(" ", ""); // 移除所有空格

        Serial.print("[Geography] Raw location string: ");
        Serial.println(location);

        // 方法1: 查找带"市"结尾的城市名
        int cityIndex = location.indexOf("市");
        bool cityFound = false;

        if (cityIndex != -1)
        {
            // 向前查找城市名称的起始位置
            int startPos = cityIndex - 1;

            // 向前查找到省份标记或国家标记
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

            // 提取城市名称 (不包括"市")
            String cityCandidate = location.substring(startPos, cityIndex);

            // 在映射表中查找
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

        // 方法2: 如果方法1失败,尝试从整个位置字符串中匹配映射表
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
