#ifndef LIGHT_CONTROLLER_H
#define LIGHT_CONTROLLER_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "mqtt_manager.h"

// NeoPixel Configuration
#define NEOPIXEL_PIN 0       // 数据引脚连接到数字引脚 0 (可用: 0,1,-2,-3,-4,-5)
#define DEFAULT_NUM_PIXELS 1 // 默认灯珠数量
#define MAX_NUM_PIXELS 16    // 最大支持灯珠数量

// Light states (底层开关)
enum LightState
{
    LIGHT_OFF, // 灯光关闭
    LIGHT_ON   // 灯光开启
};

// Light modes V2.0 (工作模式)
enum LightMode
{
    MODE_TIMER,   // 🔴 Timer模式 - 红色（预留倒计时功能）
    MODE_WEATHER, // 🟢 Weather模式 - 绿色（预留天气播报功能）
    MODE_IDLE     // 🔵 Idle模式 - 蓝色呼吸灯
};

// DEBUG mode pixel data
struct PixelDebugData
{
    uint32_t color;    // 颜色
    int brightness;    // 亮度 (0-255)
    bool isOverridden; // 是否被DEBUG模式覆盖
};

// Light Controller Class
// Manages all light control logic and MQTT interaction
class LightController
{
private:
    MQTTManager *mqtt;
    Adafruit_NeoPixel *strip; // NeoPixel 灯带对象
    int numPixels;            // 当前灯珠数量

    // V2.0 Light state
    LightState state; // 开关状态 (on/off)
    LightMode mode;   // 工作模式 (timer/weather/idle)

    // DEBUG mode data (per-pixel control)
    PixelDebugData *debugData; // 每个灯珠的DEBUG数据
    bool debugModeActive;      // DEBUG模式是否激活
    int debugSelectedIndex;    // DEBUG选中的索引 (-1 = all)

    // Idle mode (breathing) state
    unsigned long lastBreathUpdate;
    int breathDirection; // 1 = increasing, -1 = decreasing
    int breathBrightness;

    // MQTT feedback control (prevent message loop)
    bool suppressMqttFeedback;

    // V2.0 Helper functions
    void updateLEDs();
    void updateBreathingEffect();
    void applyModeColor(); // 应用模式颜色 (timer=红/weather=绿/idle=蓝)
    void setPixel(int index, uint32_t color, int brightness);
    void setAllPixels(uint32_t color, int brightness);
    uint32_t hexToColor(String hexColor);
    void parseDebugCommand(String message, int &index, String &value);

public:
    // Constructor
    LightController();

    // Destructor
    ~LightController();

    // Initialize with MQTT manager and pixel count
    void begin(MQTTManager *mqttManager, int pixelCount = DEFAULT_NUM_PIXELS);

    // Set number of pixels (supports hot-swap)
    void setNumPixels(int count);

    // Activate/Deactivate controller
    void setActive(bool active);

    // Main loop - call in Arduino loop()
    void loop();

    // MQTT message handler (V2.0)
    void handleMQTTMessage(char *topic, byte *payload, unsigned int length);

    // V2.0 Control functions
    void turnOn();                   // STATUS: on
    void turnOff();                  // STATUS: off
    void setMode(LightMode newMode); // MODE: timer/weather/idle
    void setMode(String modeName);

    // V2.0 DEBUG functions
    void debugSetColor(int index, String hexColor);     // DEBUG/color (hex string)
    void debugSetBrightness(int index, int brightness); // DEBUG/brightness
    void debugSetIndex(int index);                      // DEBUG/index
    void clearDebugMode();                              // 清除DEBUG模式

    // State publishing
    void publishState();

    // Getters
    bool isOn() { return state == LIGHT_ON; }
    LightMode getMode() { return mode; }
    int getNumPixels() { return numPixels; }
};

#endif // LIGHT_CONTROLLER_H
