#include "wifi_manager.h"
#include "geography.h"
#include "mqtt_manager.h"
#include "light_controller.h"
#include "luminaire_controller.h"
#include "weather_manager.h"
#include "button_manager.h"

#define NUM_PIXELS 8
#define SYSTEM_VERSION "2.1.0"
#define LUMINAIRE_ID "16"

enum ControllerMode
{
  MODE_LOCAL,
  MODE_LUMINAIRE
};

MQTTManager mqtt;
LightController lightControl;
LuminaireController luminaireControl;
WeatherManager weatherManager;
ButtonManager buttonManager;
String systemCity = "London";
ControllerMode currentController = MODE_LOCAL;

void mqttMessageReceived(char *topic, byte *payload, unsigned int length)
{
  String topicStr = String(topic);

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

      mqtt.publishInfo("controller", "local", true);

      const char *currentState = lightControl.getStateString();
      const char *currentMode = lightControl.getModeString();
      mqtt.publish("status", currentState, true);
      mqtt.publish("mode", currentMode, true);

      mqtt.publishInfo("lighter/number", String(lightControl.getNumPixels()).c_str(), true);

      Serial.println("[System] Switched to LOCAL controller");
      Serial.print("[System] State: ");
      Serial.print(currentState);
      Serial.print(", Mode: ");
      Serial.println(currentMode);
    }
    else if (msg == "luminaire")
    {
      currentController = MODE_LUMINAIRE;
      lightControl.setActive(false);
      luminaireControl.setActive(true);

      mqtt.publishInfo("controller", "luminaire", true);

      const char *currentState = luminaireControl.getStateString();
      const char *currentMode = luminaireControl.getModeString();
      mqtt.publish("status", currentState, true);
      mqtt.publish("mode", currentMode, true);

      mqtt.publishInfo("lighter/number", String(luminaireControl.getNumLEDs()).c_str(), true);

      Serial.println("[System] Switched to LUMINAIRE controller");
      Serial.print("[System] State: ");
      Serial.print(currentState);
      Serial.print(", Mode: ");
      Serial.println(currentMode);
    }
    return;
  }

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

  Serial.begin(9600);

  while (!Serial)
  {
    ;
  }

  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("   Aura Light System V2.1 Starting... ");
  Serial.println("========================================\n");

  setupWiFi();

  systemCity = getCurrentCity();
  Serial.println("========================================");
  Serial.print("[System] Current city: ");
  Serial.println(systemCity);
  Serial.println("========================================\n");

  mqtt.begin();
  mqtt.setCallback(mqttMessageReceived);

  if (mqtt.connect())
  {
    Serial.println("[System] ✓ MQTT connected\n");

    String controllerTopic = String(TOPIC_BASE) + "/controller";
    mqtt.subscribe(controllerTopic.c_str());
    Serial.print("[MQTT] ✓ Subscribed to: ");
    Serial.println(controllerTopic);
  }
  else
  {
    Serial.println("[System] ✗ MQTT connection failed\n");
  }

  Serial.print("[System] Initializing local controller with ");
  Serial.print(NUM_PIXELS);
  Serial.println(" NeoPixel(s)...");
  lightControl.begin(&mqtt, NUM_PIXELS);
  lightControl.setActive(true);

  Serial.println("[System] Initializing Luminaire controller...");
  luminaireControl.begin(&mqtt, LUMINAIRE_ID);
  luminaireControl.setActive(false);

  if (mqtt.isConnected())
  {
    Serial.println("\n[System] Publishing system information...");
    mqtt.publishAllInfo(NUM_PIXELS, NEOPIXEL_PIN, SYSTEM_VERSION, systemCity.c_str());

    lightControl.publishState();

    mqtt.publishInfo("controller", "local", true);
  }

  Serial.println("\n[System] Initializing weather manager...");
  weatherManager.begin(&mqtt, systemCity);

  Serial.println("\n[System] Initializing button manager on pin 1...");
  buttonManager.begin(&mqtt, &lightControl, &luminaireControl, (int *)&currentController);

  Serial.println("\n========================================");
  Serial.println("[System] ✓ System ready!");
  Serial.println("========================================\n");

  Serial.println("MQTT Control Commands (V2.1):");
  Serial.println("  CONTROLLER:");
  Serial.println("    .../controller: local / luminaire");
  Serial.println();
  Serial.println("  STATUS:");
  Serial.println("    .../status: on / off");
  Serial.println();
  Serial.println("  MODE:");
  Serial.println("    .../mode: timer (🔴) / weather (🟢) / idle (🔵) / music (⚪)");
  Serial.println();
  Serial.println("  BUTTON (Hardware on Pin 1):");
  Serial.println("    Short press: Cycle modes (Timer→Weather→Idle→Music)");
  Serial.println("    Long press (2s): Toggle light ON/OFF");
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

  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > 30000)
  {
    if (!checkWiFiConnection())
    {
      Serial.println("\n[System] ✗ WiFi connection lost!");
      reconnectWiFi();

      if (checkWiFiConnection() && !mqtt.isConnected())
      {
        Serial.println("[System] Reconnecting to MQTT...");
        if (mqtt.reconnect())
        {
          Serial.println("[System] ✓ MQTT reconnected!");

          mqtt.publishAllInfo(lightControl.getNumPixels(), NEOPIXEL_PIN, SYSTEM_VERSION, systemCity.c_str());
          lightControl.publishState();
          mqtt.publishInfo("controller", currentController == MODE_LOCAL ? "local" : "luminaire", true);
        }
      }
    }
    lastWiFiCheck = millis();
  }

  mqtt.loop();

  lightControl.loop();

  weatherManager.loop();

  buttonManager.loop();

  if (Serial.available() > 0)
  {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "republish" || command == "r")
    {
      Serial.println("\n[System] Re-publishing all states...");
      lightControl.publishState();
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

  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 300000)
  {
    if (mqtt.isConnected())
    {
      mqtt.publishInfo_System_Uptime();
    }
    lastHeartbeat = millis();
  }
}
