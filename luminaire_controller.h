#ifndef LUMINAIRE_CONTROLLER_H
#define LUMINAIRE_CONTROLLER_H

#include <Arduino.h>
#include "mqtt_manager.h"

// 前向声明
class MusicMode;
class AudioAnalyzer;
class WeatherAnimation;

#define LUMINAIRE_NUM_LEDS 72
#define LUMINAIRE_PAYLOAD_SIZE (LUMINAIRE_NUM_LEDS * 3)

enum LuminaireMode
{
    LUMI_MODE_TIMER = 0,
    LUMI_MODE_WEATHER = 1,
    LUMI_MODE_IDLE = 2,
    LUMI_MODE_MUSIC = 3
};

enum LuminaireState
{
    LUMI_OFF = 0,
    LUMI_ON = 1
};

class LuminaireController
{
private:
    MQTTManager *mqtt;
    MusicMode *musicMode;         // Music 模式引用
    AudioAnalyzer *audioAnalyzer; // 音频分析器引用
    WeatherAnimation *weatherAnim; // 天气动画引用

    String lightId;
    String mqttTopic;
    byte RGBpayload[LUMINAIRE_PAYLOAD_SIZE];

    bool isActive;
    LuminaireState state;
    LuminaireMode mode;

    uint32_t idleColor; // 自定义 IDLE 模式颜色

    // 呼吸灯效果变量
    unsigned long lastBreathUpdate;
    int breathDirection;
    int breathBrightness;

    // 天气可视化相关变量
    float currentTemp;    // 当前温度
    float feelsLikeTemp;  // 体感温度
    int humidity;         // 湿度
    int windSpeed;        // 风速 (km/h)
    String windDirection; // 风向
    int visibility;       // 能见度 (km)
    int cloudCover;       // 云量 (%)
    float precipitation;  // 降水量 (mm)
    String weatherCode;   // 天气代码
    String weatherDesc;   // 天气描述

    unsigned long lastWeatherUpdate;
    unsigned long lastWindUpdate;
    int windAnimationOffset; // 风速动画偏移
    
    // 天气动画切换控制
    bool showingAnimation;           // 当前是否显示动画
    unsigned long lastModeSwitch;    // 上次模式切换时间
    static const unsigned long DISPLAY_DURATION = 5000; // 每个模式显示5秒

    void applyModeColor();
    void updateMusicSpectrum();        // 新增：更新 Music 频谱显示
    void updateBreathingEffect();      // 新增：更新 IDLE 呼吸灯效果
    void updateWeatherVisualization(); // 新增：更新天气可视化
    void getRGBFromHex(const String &hexColor, int &r, int &g, int &b);

    // 伞状LED映射工具函数
    int getUmbrellaLED(int rib, int position);                         // 获取指定伞骨和位置的LED编号
    void setUmbrellaPixel(int rib, int position, int r, int g, int b); // 设置单个LED
    void setRadialRing(int position, int r, int g, int b);             // 设置径向环（所有伞骨的同一位置）

    // 天气可视化渲染函数（新的6行设计）
    void renderHumidity();         // 第一行：湿度（蓝色，越大越亮）
    void renderWindSpeed();        // 第二行：风速（白色追逐光点）
    void renderVisibility();       // 第三行：可见度（白色，5-20km映射）
    void renderTemperature();      // 第四行：当前温度（白/蓝/绿/黄/红渐变）
    void renderFeelsLike();        // 第五行：体感温度（闪烁aqua或橙黄）
    void renderCloudCover();       // 第六行：云量（棕色，越多越深）
    void renderWeatherAnimation(); // 占位函数（已移除）

public:
    LuminaireController();
    void begin(MQTTManager *mqttManager, const String &id);
    void setActive(bool active);
    bool getActive() const { return isActive; }

    // 设置 Music 模式和音频分析器
    void setMusicMode(MusicMode *music, AudioAnalyzer *audio);
    
    // 设置天气动画
    void setWeatherAnimation(WeatherAnimation *anim);

    // 主循环（用于 Music 模式更新）
    void loop();

    void handleMQTTMessage(char *topic, byte *payload, unsigned int length);

    void sendRGBToPixel(int r, int g, int b, int pixel);

    void sendRGBToAll(int r, int g, int b);
    
    // 批量更新所有LED（用于动画）
    void updateAllLEDs(byte *data, int size);

    void clear();

    // 天气数据更新接口
    void updateWeatherData(const String &weatherJson); // 更新天气数据

    int getNumLEDs() const { return LUMINAIRE_NUM_LEDS; }

    bool isOn() const { return state == LUMI_ON; }
    LuminaireMode getMode() const { return mode; }
    const char *getModeString() const
    {
        const char *modeNames[] = {"timer", "weather", "idle", "music"};
        return modeNames[mode];
    }
    const char *getStateString() const
    {
        return (state == LUMI_ON) ? "on" : "off";
    }

    // 设置和获取 IDLE 颜色
    void setIdleColor(uint32_t color) { idleColor = color; }
    String getIdleColor() const
    {
        char hex[8];
        sprintf(hex, "#%02X%02X%02X",
                (uint8_t)((idleColor >> 16) & 0xFF),
                (uint8_t)((idleColor >> 8) & 0xFF),
                (uint8_t)(idleColor & 0xFF));
        return String(hex);
    }
};

#endif
