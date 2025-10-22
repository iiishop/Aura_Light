#include "wifi_manager.h"
#include "arduino_secrets.h"

void setupWiFi()
{
    
    if (WiFi.status() == WL_NO_MODULE)
    {
        Serial.println("[WiFi] Communication with WiFi module failed!");
        Serial.println("[WiFi] System halted.");
        while (true)
            ;
    }

    
    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION)
    {
        Serial.println("[WiFi] Please upgrade the firmware");
    }

    Serial.println("========================================");
    Serial.println("WiFi Connection Starting...");
    Serial.println("========================================");
    Serial.print("[WiFi] Configured networks: ");
    Serial.println(WIFI_NETWORK_COUNT);

    
    for (int i = 0; i < WIFI_NETWORK_COUNT; i++)
    {
        Serial.print("  ");
        Serial.print(i + 1);
        Serial.print(". ");
        Serial.println(WIFI_NETWORKS[i].ssid);
    }
    Serial.println();

    
    Serial.println("[WiFi] Scanning for available networks...");
    int numNetworks = WiFi.scanNetworks();
    Serial.print("[WiFi] Found ");
    Serial.print(numNetworks);
    Serial.println(" networks:");

    
    for (int i = 0; i < numNetworks; i++)
    {
        Serial.print("  ");
        Serial.print(i + 1);
        Serial.print(". ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(" dBm, ");

        
        byte encryption = WiFi.encryptionType(i);
        if (encryption == ENC_TYPE_WEP)
        {
            Serial.print("WEP");
        }
        else if (encryption == ENC_TYPE_TKIP)
        {
            Serial.print("WPA");
        }
        else if (encryption == ENC_TYPE_CCMP)
        {
            Serial.print("WPA2");
        }
        else if (encryption == ENC_TYPE_NONE)
        {
            Serial.print("OPEN");
        }
        else if (encryption == ENC_TYPE_AUTO)
        {
            Serial.print("AUTO");
        }
        else
        {
            Serial.print("UNKNOWN");
        }
        Serial.println(")");
    }
    Serial.println();

    
    Serial.println("[WiFi] Matching configured networks with scan results...");

    struct MatchedNetwork
    {
        int configIndex;
        int scanIndex;
        int rssi;
    };

    MatchedNetwork matchedNetworks[WIFI_NETWORK_COUNT];
    int matchCount = 0;

    
    for (int i = 0; i < WIFI_NETWORK_COUNT; i++)
    {
        for (int j = 0; j < numNetworks; j++)
        {
            String scannedSSID = WiFi.SSID(j);
            String configSSID = String(WIFI_NETWORKS[i].ssid);

            
            scannedSSID.trim();
            configSSID.trim();

            if (scannedSSID.equals(configSSID))
            {
                matchedNetworks[matchCount].configIndex = i;
                matchedNetworks[matchCount].scanIndex = j;
                matchedNetworks[matchCount].rssi = WiFi.RSSI(j);
                matchCount++;

                Serial.print("[WiFi] ✓ Found: ");
                Serial.print(WIFI_NETWORKS[i].ssid);
                Serial.print(" (Signal: ");
                Serial.print(WiFi.RSSI(j));
                Serial.println(" dBm)");
                break;
            }
        }
    }

    if (matchCount == 0)
    {
        Serial.println("[WiFi] ✗ No configured networks found in scan!");
        Serial.println("[WiFi] Please check:");
        Serial.println("[WiFi]   1. Network names are correct");
        Serial.println("[WiFi]   2. Networks are powered on");
        Serial.println("[WiFi]   3. Device is in range");
        return;
    }

    Serial.print("[WiFi] Matched ");
    Serial.print(matchCount);
    Serial.print(" network(s) out of ");
    Serial.println(WIFI_NETWORK_COUNT);
    Serial.println();

    
    for (int i = 0; i < matchCount - 1; i++)
    {
        for (int j = i + 1; j < matchCount; j++)
        {
            if (matchedNetworks[j].rssi > matchedNetworks[i].rssi)
            {
                MatchedNetwork temp = matchedNetworks[i];
                matchedNetworks[i] = matchedNetworks[j];
                matchedNetworks[j] = temp;
            }
        }
    }

    Serial.println("[WiFi] Networks ordered by signal strength:");
    for (int i = 0; i < matchCount; i++)
    {
        Serial.print("  ");
        Serial.print(i + 1);
        Serial.print(". ");
        Serial.print(WIFI_NETWORKS[matchedNetworks[i].configIndex].ssid);
        Serial.print(" (");
        Serial.print(matchedNetworks[i].rssi);
        Serial.println(" dBm)");
    }
    Serial.println();

    
    int status = WL_IDLE_STATUS;
    bool connected = false;

    for (int i = 0; i < matchCount && !connected; i++)
    {
        int configIdx = matchedNetworks[i].configIndex;
        const char *currentSSID = WIFI_NETWORKS[configIdx].ssid;
        const char *currentPass = WIFI_NETWORKS[configIdx].password;
        int signalStrength = matchedNetworks[i].rssi;

        Serial.print("[WiFi] Connecting to network ");
        Serial.print(i + 1);
        Serial.print("/");
        Serial.print(matchCount);
        Serial.print(": ");
        Serial.print(currentSSID);
        Serial.print(" (");
        Serial.print(signalStrength);
        Serial.println(" dBm)");

        
        const int MAX_ATTEMPTS = 3;

        for (int attempt = 0; attempt < MAX_ATTEMPTS && !connected; attempt++)
        {
            Serial.print("  Attempt ");
            Serial.print(attempt + 1);
            Serial.print("/");
            Serial.print(MAX_ATTEMPTS);
            Serial.print("...");

            
            status = WiFi.begin(currentSSID, currentPass);

            
            for (int j = 0; j < 8; j++)
            {
                delay(1000);
                Serial.print(".");
            }
            Serial.println();

            
            status = WiFi.status();

            if (status == WL_CONNECTED)
            {
                connected = true;
                Serial.println("  ✓ Connection successful!");
            }
            else
            {
                Serial.print("  ✗ Failed. Status: ");
                Serial.println(status);
            }
        }

        if (!connected)
        {
            Serial.print("[WiFi] Could not connect to ");
            Serial.println(currentSSID);
            Serial.println();
        }
    }

    
    if (connected)
    {
        Serial.println("========================================");
        Serial.println("WiFi Connected Successfully!");
        Serial.println("========================================");
        Serial.print("[WiFi] Connected to SSID: ");
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
        Serial.println("[WiFi] ✗ Failed to connect to any network");
        Serial.println("[WiFi] Please check:");
        Serial.println("[WiFi]   1. WiFi credentials in arduino_secrets.h");
        Serial.println("[WiFi]   2. WiFi networks are available and in range");
        Serial.println("[WiFi]   3. WiFi signal strength is sufficient");
        Serial.println("========================================\n");
    }
}

bool checkWiFiConnection()
{
    return WiFi.status() == WL_CONNECTED;
}

void reconnectWiFi()
{
    Serial.println("\n========================================");
    Serial.println("[WiFi] Connection lost! Attempting to reconnect...");
    Serial.println("========================================");

    
    WiFi.disconnect();
    delay(1000);

    
    Serial.println("[WiFi] Scanning for available networks...");
    int numNetworks = WiFi.scanNetworks();

    if (numNetworks == 0)
    {
        Serial.println("[WiFi] ✗ No networks found in scan!");
        return;
    }

    Serial.print("[WiFi] Found ");
    Serial.print(numNetworks);
    Serial.println(" networks");

    
    struct MatchedNetwork
    {
        int configIndex;
        int rssi;
    };

    MatchedNetwork matchedNetworks[WIFI_NETWORK_COUNT];
    int matchCount = 0;

    for (int i = 0; i < WIFI_NETWORK_COUNT; i++)
    {
        for (int j = 0; j < numNetworks; j++)
        {
            String scannedSSID = WiFi.SSID(j);
            String configSSID = String(WIFI_NETWORKS[i].ssid);

            scannedSSID.trim();
            configSSID.trim();

            if (scannedSSID.equals(configSSID))
            {
                matchedNetworks[matchCount].configIndex = i;
                matchedNetworks[matchCount].rssi = WiFi.RSSI(j);
                matchCount++;

                Serial.print("[WiFi] ✓ Found: ");
                Serial.print(WIFI_NETWORKS[i].ssid);
                Serial.print(" (");
                Serial.print(WiFi.RSSI(j));
                Serial.println(" dBm)");
                break;
            }
        }
    }

    if (matchCount == 0)
    {
        Serial.println("[WiFi] ✗ No configured networks available!");
        return;
    }

    
    for (int i = 0; i < matchCount - 1; i++)
    {
        for (int j = i + 1; j < matchCount; j++)
        {
            if (matchedNetworks[j].rssi > matchedNetworks[i].rssi)
            {
                MatchedNetwork temp = matchedNetworks[i];
                matchedNetworks[i] = matchedNetworks[j];
                matchedNetworks[j] = temp;
            }
        }
    }

    
    int configIdx = matchedNetworks[0].configIndex;
    const char *ssid = WIFI_NETWORKS[configIdx].ssid;
    const char *pass = WIFI_NETWORKS[configIdx].password;

    Serial.print("[WiFi] Reconnecting to: ");
    Serial.print(ssid);
    Serial.print(" (");
    Serial.print(matchedNetworks[0].rssi);
    Serial.println(" dBm)");

    int status = WiFi.begin(ssid, pass);

    
    for (int i = 0; i < 10; i++)
    {
        delay(1000);
        Serial.print(".");
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println();
            Serial.println("[WiFi] ✓ Reconnection successful!");
            Serial.print("[WiFi] IP Address: ");
            Serial.println(WiFi.localIP());
            Serial.print("[WiFi] Signal: ");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm");
            Serial.println("========================================\n");
            return;
        }
    }

    Serial.println();
    Serial.println("[WiFi] ✗ Reconnection failed!");
    Serial.println("========================================\n");
}
