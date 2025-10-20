#include "wifi_manager.h"
#include "geography.h"
#include "mqtt_manager.h"
#include "light_controller.h"
#include "weather_manager.h"

// ============ CONFIGURATION ============
// 设置连接的灯珠数量 (1-无限制)
// Set the number of NeoPixel LEDs connected (1-unlimited)
#define NUM_PIXELS 1           // ← 修改这里来改变灯珠数量
#define SYSTEM_VERSION "2.0.0" // V2.0版本
// =======================================

// Create instances
MQTTManager mqtt;
LightController lightControl;
WeatherManager weatherManager;
String systemCity = "London"; // 默认城市，在setup()中更新

// MQTT message callback - forward to light controller
void mqttMessageReceived(char *topic, byte *payload, unsigned int length)
{
  lightControl.handleMQTTMessage(topic, payload, length);
}

void setup()
{
  // Initialize serial communication
  Serial.begin(9600);

  // Wait for serial port to connect
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("   Aura Light System V2.0 Starting... ");
  Serial.println("========================================\n");

  // 1. Connect to WiFi
  setupWiFi();

  // 2. Get current location
  systemCity = getCurrentCity();
  Serial.println("========================================");
  Serial.print("[System] Current city: ");
  Serial.println(systemCity);
  Serial.println("========================================\n");

  // 3. Initialize MQTT
  mqtt.begin();
  mqtt.setCallback(mqttMessageReceived);

  if (mqtt.connect())
  {
    Serial.println("[System] ✓ MQTT connected\n");
  }
  else
  {
    Serial.println("[System] ✗ MQTT connection failed\n");
  }

  // 4. Initialize Light Controller
  Serial.print("[System] Initializing with ");
  Serial.print(NUM_PIXELS);
  Serial.println(" NeoPixel(s)...");
  lightControl.begin(&mqtt, NUM_PIXELS);

  // 5. V2.0: Publish all INFO topics
  if (mqtt.isConnected())
  {
    Serial.println("\n[System] Publishing system information...");
    mqtt.publishAllInfo(NUM_PIXELS, NEOPIXEL_PIN, SYSTEM_VERSION, systemCity.c_str());

    // Publish initial state
    lightControl.publishState();
  }

  // 6. Initialize Weather Manager
  Serial.println("\n[System] Initializing weather manager...");
  weatherManager.begin(&mqtt, systemCity);

  Serial.println("\n========================================");
  Serial.println("[System] ✓ System ready!");
  Serial.println("========================================\n");

  // V2.0: Print control instructions
  Serial.println("MQTT Control Commands (V2.0):");
  Serial.println("  STATUS:");
  Serial.println("    .../status: on / off");
  Serial.println();
  Serial.println("  MODE:");
  Serial.println("    .../mode: timer (🔴) / weather (🟢) / idle (🔵)");
  Serial.println();
  Serial.println("  DEBUG:");
  Serial.println("    .../debug/color: 0:#FF0000 (pixel 0 = red)");
  Serial.println("    .../debug/brightness: 1:128 (pixel 1 = 50%)");
  Serial.println("    .../debug/index: clear (clear all DEBUG)");
  Serial.println();
  Serial.println("  INFO (read-only, auto-published):");
  Serial.println("    .../info/wifi/*");
  Serial.println("    .../info/lighter/*");
  Serial.println("    .../info/system/*");
  Serial.println("    .../info/location/*");
  Serial.println();
}

void loop()
{
  // Maintain MQTT connection
  mqtt.loop();

  // Update light controller (handles breathing effects in IDLE mode)
  lightControl.loop();

  // Update weather manager (auto-fetch every 10 minutes)
  weatherManager.loop();

  // Check for serial commands
  if (Serial.available() > 0)
  {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "republish" || command == "r")
    {
      Serial.println("\n[System] Re-publishing all states...");
      lightControl.publishState(); // 重新发布 status 和 mode
      Serial.println("[System] ✓ States re-published!");
    }
    else if (command == "info" || command == "i")
    {
      Serial.println("\n[System] Re-publishing all INFO...");
      mqtt.publishAllInfo(lightControl.getNumPixels(), NEOPIXEL_PIN, SYSTEM_VERSION, systemCity.c_str());
      Serial.println("[System] ✓ INFO re-published!");
    }
    else if (command == "help" || command == "h")
    {
      Serial.println("\n=== Serial Commands ===");
      Serial.println("  r / republish  - Re-publish status and mode");
      Serial.println("  i / info       - Re-publish all INFO");
      Serial.println("  h / help       - Show this help");
      Serial.println("=======================\n");
    }
  }

  // V2.0: Heartbeat - publish uptime every 5 minutes
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 300000) // 5 minutes
  {
    if (mqtt.isConnected())
    {
      mqtt.publishInfo_System_Uptime(); // 自动计算uptime
    }
    lastHeartbeat = millis();
  }
}
