#include "wifi_manager.h"
#include "geography.h"
#include "mqtt_manager.h"
#include "light_controller.h"
#include "luminaire_controller.h"
#include "weather_manager.h"
#include "button_manager.h"
#include "music_mode.h"
#include "audio_analyzer.h"

#define NUM_PIXELS 8
#define SYSTEM_VERSION "2.2.0"
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
MusicMode musicMode;
AudioAnalyzer audioAnalyzer;
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

      // 更新状态LED（Local模式下熄灭）
      buttonManager.updateStatusLED();
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

      // 更新状态LED（显示Luminaire状态）
      buttonManager.updateStatusLED();
    }
    return;
  }

  // 音频系统设置（全局，不分控制器）
  if (topicStr.endsWith("/audio/volume_range"))
  {
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    String msg = String(message);

    // 期望格式: "30,120" (minDb,maxDb)
    int commaIndex = msg.indexOf(',');
    if (commaIndex > 0)
    {
      float minDb = msg.substring(0, commaIndex).toFloat();
      float maxDb = msg.substring(commaIndex + 1).toFloat();

      if (minDb >= 20 && minDb < maxDb && maxDb <= 130)
      {
        audioAnalyzer.setVolumeRange(minDb, maxDb);
        mqtt.publishInfo("audio/volume_range", msg.c_str(), true);

        Serial.print("[Audio] Volume range updated: ");
        Serial.print(minDb);
        Serial.print(" - ");
        Serial.print(maxDb);
        Serial.println(" dB");
      }
      else
      {
        Serial.println("[Audio] Invalid volume range (must be 20-130 dB)");
      }
    }
    return;
  }

  // IDLE 颜色设置（全局，应用到两个控制器）
  if (topicStr.endsWith("/idle/color"))
  {
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    String colorStr = String(message);

    // 验证颜色格式 #RRGGBB
    if (colorStr.length() == 7 && colorStr.charAt(0) == '#')
    {
      // 转换为 uint32_t
      long colorValue = strtol(colorStr.substring(1).c_str(), NULL, 16);
      uint32_t color = (uint32_t)colorValue;

      // 同时应用到两个控制器
      // 1. Local controller 通过它自己的 handleMQTTMessage 处理
      lightControl.handleMQTTMessage(topic, payload, length);

      // 2. Luminaire controller 直接设置颜色
      luminaireControl.setIdleColor(color);

      // 如果 Luminaire 当前是 IDLE 模式且开启，立即更新显示
      if (luminaireControl.isOn() && luminaireControl.getMode() == LUMI_MODE_IDLE)
      {
        // 重新应用 IDLE 颜色
        int r = (color >> 16) & 0xFF;
        int g = (color >> 8) & 0xFF;
        int b = color & 0xFF;
        luminaireControl.sendRGBToAll(r, g, b);

        Serial.print("[Luminaire] IDLE color updated to: ");
        Serial.println(colorStr);
      }

      Serial.print("[System] IDLE color set to: ");
      Serial.print(colorStr);
      Serial.println(" (applied to both controllers)");
    }
    else
    {
      Serial.print("[System] Invalid IDLE color format: ");
      Serial.println(colorStr);
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
    // Luminaire 状态/模式可能已改变，更新状态LED
    buttonManager.updateStatusLED();
  }
}

// 开机进度条 - 基于启动阶段
Adafruit_NeoPixel *bootStrip = nullptr;
int currentBootProgress = 0;
const int TOTAL_BOOT_STAGES = NUM_PIXELS; // 使用灯珠数量作为总阶段数

void initBootProgressBar()
{
  // 创建 NeoPixel 对象用于开机动画
  bootStrip = new Adafruit_NeoPixel(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
  bootStrip->begin();
  bootStrip->setBrightness(100); // 中等亮度

  // 全部点亮为蓝色（表示等待/初始化）
  for (int i = 0; i < NUM_PIXELS; i++)
  {
    bootStrip->setPixelColor(i, bootStrip->Color(0, 50, 255));
  }
  bootStrip->show();
  currentBootProgress = 0;

  Serial.println("[Boot] Progress bar initialized");
}

void updateBootProgress(const char *stage)
{
  if (bootStrip == nullptr || currentBootProgress >= TOTAL_BOOT_STAGES)
    return;

  Serial.print("[Boot Progress] ");
  Serial.print(currentBootProgress + 1);
  Serial.print("/");
  Serial.print(TOTAL_BOOT_STAGES);
  Serial.print(" - ");
  Serial.println(stage);

  // 将当前进度的灯珠变为绿色（已完成）
  bootStrip->setPixelColor(currentBootProgress, bootStrip->Color(0, 255, 0));
  bootStrip->show();

  currentBootProgress++;
  delay(100); // 短暂延迟以便看清变化
}

void finishBootProgress()
{
  if (bootStrip == nullptr)
    return;

  Serial.println("[Boot] Finishing progress bar...");

  // 所有灯珠变为绿色（全部完成）
  for (int i = 0; i < NUM_PIXELS; i++)
  {
    bootStrip->setPixelColor(i, bootStrip->Color(0, 255, 0));
  }
  bootStrip->show();
  delay(500);

  // 渐变到暗
  for (int brightness = 255; brightness >= 0; brightness -= 15)
  {
    for (int i = 0; i < NUM_PIXELS; i++)
    {
      bootStrip->setPixelColor(i, bootStrip->Color(0, brightness, 0));
    }
    bootStrip->show();
    delay(30);
  }

  // 清除并释放资源
  bootStrip->clear();
  bootStrip->show();
  delete bootStrip;
  bootStrip = nullptr;

  Serial.println("[Boot] Progress bar completed and cleared");
}

void setup()
{

  Serial.begin(9600);

  // 初始化开机进度条
  initBootProgressBar();
  updateBootProgress("Serial initialized");

  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("   Aura Light System V2.1 Starting... ");
  Serial.println("========================================\n");

  setupWiFi();
  updateBootProgress("WiFi connected");

  systemCity = getCurrentCity();
  updateBootProgress("Location acquired");
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
  updateBootProgress("MQTT initialized");

  Serial.print("[System] Initializing local controller with ");
  Serial.print(NUM_PIXELS);
  Serial.println(" NeoPixel(s)...");
  lightControl.begin(&mqtt, NUM_PIXELS);
  lightControl.setActive(true);

  Serial.println("[System] Initializing Luminaire controller...");
  luminaireControl.begin(&mqtt, LUMINAIRE_ID);
  luminaireControl.setActive(false);

  // 同步两个控制器的 IDLE 颜色（使用 lightControl 的默认颜色）
  String defaultIdleColor = lightControl.getIdleColor();
  long colorValue = strtol(defaultIdleColor.substring(1).c_str(), NULL, 16);
  luminaireControl.setIdleColor((uint32_t)colorValue);
  Serial.print("[System] IDLE color synchronized: ");
  Serial.println(defaultIdleColor);

  updateBootProgress("Controllers ready");

  // 初始化音频分析器
  Serial.println("\n[System] Initializing audio analyzer (MAX9814 on A0)...");
  audioAnalyzer.begin();

  // 初始化 Music 模式
  Serial.println("[System] Initializing music mode...");
  musicMode.begin(&audioAnalyzer);

  // 配置控制器的 Music 模式
  lightControl.setMusicMode(&musicMode, &audioAnalyzer);
  luminaireControl.setMusicMode(&musicMode, &audioAnalyzer);
  updateBootProgress("Audio initialized");

  if (mqtt.isConnected())
  {
    Serial.println("\n[System] Publishing system information...");
    mqtt.publishAllInfo(NUM_PIXELS, NEOPIXEL_PIN, SYSTEM_VERSION, systemCity.c_str());

    lightControl.publishState();

    mqtt.publishInfo("controller", "local", true);

    // 发布 IDLE 模式的初始颜色
    mqtt.publishInfo("idle/color", lightControl.getIdleColor().c_str(), true);
  }

  Serial.println("\n[System] Initializing weather manager...");
  weatherManager.begin(&mqtt, systemCity);
  updateBootProgress("Weather ready");

  Serial.println("\n[System] Initializing button manager on pin 1...");
  buttonManager.begin(&mqtt, &lightControl, &luminaireControl, (int *)&currentController);
  updateBootProgress("Button ready");

  Serial.println("\n========================================");
  Serial.println("[System] ✓ System ready!");
  Serial.println("========================================\n");

  // 完成开机进度条
  finishBootProgress();

  // 重新初始化 lightControl 的 NeoPixel (因为进度条使用了同一引脚)
  Serial.println("[System] Reinitializing local controller NeoPixel after boot progress...");
  lightControl.begin(&mqtt, NUM_PIXELS);
  lightControl.setActive(true);

  Serial.println("MQTT Control Commands (V2.2):");
  Serial.println("  CONTROLLER:");
  Serial.println("    .../controller: local / luminaire");
  Serial.println();
  Serial.println("  STATUS:");
  Serial.println("    .../status: on / off");
  Serial.println();
  Serial.println("  MODE:");
  Serial.println("    .../mode: timer / weather / idle / music");
  Serial.println();
  Serial.println("  MUSIC MODE (Audio Visualization):");
  Serial.println("    Local: 8-level VU meter (volume bars)");
  Serial.println("    Luminaire: 12x6 virtual spectrum (frequency bands)");
  Serial.println("    Hardware: MAX9814 microphone on A0");
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

  // MQTT 循环
  mqtt.loop();

  // 音频分析器循环（持续采样）
  audioAnalyzer.loop();

  // 控制器循环
  lightControl.loop();
  luminaireControl.loop(); // 新增：Luminaire Music 模式更新

  // 天气管理器循环
  weatherManager.loop();

  // 按钮管理器循环
  buttonManager.loop();

  // 定期更新状态LED（Luminaire模式下）
  static unsigned long lastStatusUpdate = 0;
  if (currentController == MODE_LUMINAIRE && millis() - lastStatusUpdate > 1000)
  {
    buttonManager.updateStatusLED();
    lastStatusUpdate = millis();
  }

  // 串口命令处理
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

  // 发布音频数据到 Dashboard（每 200ms）
  static unsigned long lastAudioPublish = 0;
  if (millis() - lastAudioPublish > 200)
  {
    if (mqtt.isConnected())
    {
      // 获取音频数据
      int rawADC = audioAnalyzer.getRawADC();
      float volumeDb = audioAnalyzer.getVolumeDecibel();
      int vuLevel = musicMode.getVULevel();

      // 获取虚拟频谱数据
      float spectrum[12];
      musicMode.getSpectrumData(spectrum);

      // 构建消息: "raw,volume,vuLevel,band0,band1,...,band11"
      String audioMsg = String(rawADC) + "," + String(volumeDb, 1) + "," + String(vuLevel);
      for (int i = 0; i < 12; i++)
      {
        audioMsg += "," + String(spectrum[i], 2);
      }

      // 发布到 MQTT
      mqtt.publishInfo("audio/data", audioMsg.c_str(), false);
    }
    lastAudioPublish = millis();
  }
}
