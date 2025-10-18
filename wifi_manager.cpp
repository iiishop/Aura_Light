#include "wifi_manager.h"
#include "arduino_secrets.h"

void setupWiFi()
{
    // Check for WiFi module
    if (WiFi.status() == WL_NO_MODULE)
    {
        Serial.println("[WiFi] Communication with WiFi module failed!");
        Serial.println("[WiFi] System halted.");
        while (true)
            ;
    }

    // Check firmware version
    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION)
    {
        Serial.println("[WiFi] Please upgrade the firmware");
    }

    // Attempt to connect to WiFi network
    int status = WL_IDLE_STATUS;
    int attempts = 0;
    const int MAX_ATTEMPTS = 10;

    Serial.println("========================================");
    Serial.println("WiFi Connection Starting...");
    Serial.println("========================================");

    while (status != WL_CONNECTED && attempts < MAX_ATTEMPTS)
    {
        attempts++;
        Serial.print("[WiFi] Attempt ");
        Serial.print(attempts);
        Serial.print("/");
        Serial.print(MAX_ATTEMPTS);
        Serial.print(" - Connecting to SSID: ");
        Serial.println(SECRET_SSID);

        // Connect to WPA/WPA2 network
        status = WiFi.begin(SECRET_SSID, SECRET_PASS);

        // Wait 10 seconds for connection
        for (int i = 0; i < 10; i++)
        {
            delay(1000);
            Serial.print(".");
        }
        Serial.println();

        // Check connection status
        status = WiFi.status();
        if (status == WL_CONNECTED)
        {
            Serial.println("[WiFi] ✓ Connection successful!");
        }
        else
        {
            Serial.print("[WiFi] ✗ Connection failed. Status code: ");
            Serial.println(status);
        }
    }

    // Final status check
    if (status == WL_CONNECTED)
    {
        Serial.println("========================================");
        Serial.println("WiFi Connected Successfully!");
        Serial.println("========================================");
        Serial.print("[WiFi] SSID: ");
        Serial.println(WiFi.SSID());
        Serial.print("[WiFi] IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("[WiFi] Signal Strength (RSSI): ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        Serial.println("========================================\n");
    }
    else
    {
        Serial.println("========================================");
        Serial.println("[WiFi] ✗ Failed to connect after maximum attempts");
        Serial.println("[WiFi] Please check:");
        Serial.println("[WiFi]   1. SSID and password in arduino_secrets.h");
        Serial.println("[WiFi]   2. WiFi network is available");
        Serial.println("[WiFi]   3. WiFi signal strength");
        Serial.println("========================================\n");
    }
}
