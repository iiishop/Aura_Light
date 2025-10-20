#include "wifi_manager.h"
#include "geography.h"
#include "mqtt_manager.h"
#include "light_controller.h"
#include "luminaire_controller.h"
#include "weather_manager.h"

// ============ CONFIGURATION ============
// 设置连接的灯珠数量 (1-无限制)
// Set the number of NeoPixel LEDs connected (1-unlimited)
#define NUM_PIXELS 8           // ← 修改这里来改变灯珠数量
#define SYSTEM_VERSION "2.1.0" // V2.1版本 - 添加Luminaire支持
#define LUMINAIRE_ID "16"      // Luminaire light ID
// =======================================

// Light mode selection
enum ControllerMode
{
  MODE_LOCAL,    // 控制本地实体灯 (MKR1010上的NeoPixel)
  MODE_LUMINAIRE // 控制外接Luminaire (72 LEDs)
};

// Create instances
MQTTManager mqtt;
LightController lightControl;
LuminaireController luminaireControl;
WeatherManager weatherManager;
String systemCity = "London";                  // 默认城市，在setup()中更新
ControllerMode currentController = MODE_LOCAL; // 默认控制本地灯

// MQTT message callback - forward to appropriate controller
void mqttMessageReceived(char *topic, byte *payload, unsigned int length)
{
  String topicStr = String(topic);

  // Check if this is a controller switch command
  if (topicStr.endsWith("/controller"))
  {
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    String msg = String(message);
    msg.toLowerCase();

    if (msg == "local")
    {
      currentController = MODE_LOCAL;
      lightControl.setActive(true);
      luminaireControl.setActive(false);
      mqtt.publishInfo("controller", "local", true); // Publish retained
      Serial.println("[System] Switched to LOCAL controller");
    }
    else if (msg == "luminaire")
    {
      currentController = MODE_LUMINAIRE;
      lightControl.setActive(false);
      luminaireControl.setActive(true);
      mqtt.publishInfo("controller", "luminaire", true); // Publish retained
      Serial.println("[System] Switched to LUMINAIRE controller");
    }
    return;
  }

  // Forward to active controller
  if (currentController == MODE_LOCAL)
  {
    lightControl.handleMQTTMessage(topic, payload, length);
  }
  else if (currentController == MODE_LUMINAIRE)
  {
    luminaireControl.handleMQTTMessage(topic, payload, length);
  }
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
  Serial.println("   Aura Light System V2.1 Starting... ");
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

    // Subscribe to controller switch topic
    String controllerTopic = String(TOPIC_BASE) + "/controller";
    mqtt.subscribe(controllerTopic.c_str());
    Serial.print("[MQTT] ✓ Subscribed to: ");
    Serial.println(controllerTopic);
  }
  else
  {
    Serial.println("[System] ✗ MQTT connection failed\n");
  }

  // 4. Initialize Local Light Controller
  Serial.print("[System] Initializing local controller with ");
  Serial.print(NUM_PIXELS);
  Serial.println(" NeoPixel(s)...");
  lightControl.begin(&mqtt, NUM_PIXELS);
  lightControl.setActive(true); // Default to local controller

  // 5. Initialize Luminaire Controller
  Serial.println("[System] Initializing Luminaire controller...");
  luminaireControl.begin(&mqtt, LUMINAIRE_ID);
  luminaireControl.setActive(false); // Not active by default

  // 6. V2.1: Publish all INFO topics
  if (mqtt.isConnected())
  {
    Serial.println("\n[System] Publishing system information...");
    mqtt.publishAllInfo(NUM_PIXELS, NEOPIXEL_PIN, SYSTEM_VERSION, systemCity.c_str());

    // Publish initial state
    lightControl.publishState();

    // Publish controller mode
    mqtt.publishInfo("controller", "local", true);
  }

  // 7. Initialize Weather Manager
  Serial.println("\n[System] Initializing weather manager...");
  weatherManager.begin(&mqtt, systemCity);

  Serial.println("\n========================================");
  Serial.println("[System] ✓ System ready!");
  Serial.println("========================================\n");

  // V2.1: Print control instructions
  Serial.println("MQTT Control Commands (V2.1):");
  Serial.println("  CONTROLLER:");
  Serial.println("    .../controller: local / luminaire");
  Serial.println();
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
