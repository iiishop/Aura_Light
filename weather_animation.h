#ifndef WEATHER_ANIMATION_H
#define WEATHER_ANIMATION_H

#include <Arduino.h>

// 前向声明
class LuminaireController;

// 天气代码分类
enum WeatherType
{
    WEATHER_SUNNY,      // 晴天 (113)
    WEATHER_CLOUDY,     // 多云 (116, 119, 122)
    WEATHER_RAIN,       // 雨天
    WEATHER_SNOW,       // 雪天
    WEATHER_THUNDERSTORM, // 雷暴 (200, 386-395)
    WEATHER_FOG,        // 雾天 (143, 248, 260)
    WEATHER_UNKNOWN     // 未知
};

// 雨滴结构
struct Raindrop
{
    int rib;           // 伞骨编号
    float position;    // 当前位置 (0.0-11.0)
    float speed;       // 下落速度
    bool active;       // 是否激活
    unsigned long lastUpdate; // 上次更新时间
};

// 雪花结构
struct Snowflake
{
    int rib;           // 当前伞骨
    float position;    // 当前位置
    float speed;       // 下落速度
    bool active;
    unsigned long lastUpdate;
    int swayDirection; // 摇摆方向 (-1, 0, 1)
};

// 闪电结构
struct Lightning
{
    int ribs[2];       // 闪电影响的伞骨
    int ribCount;      // 闪电数量(1-2)
    float progress;    // 进度 (0.0-1.0)
    bool active;
    unsigned long startTime;
};

class WeatherAnimation
{
private:
    LuminaireController *controller;
    
    // 本地LED缓存（避免频繁MQTT发送）
    byte localBuffer[72 * 3]; // 72个LED，每个3字节RGB
    
    // 天气数据
    String weatherCode;
    WeatherType weatherType;
    int cloudCover;      // 云量 (%)
    float precipitation; // 降水量 (mm)
    int visibility;      // 能见度 (km)
    
    // 动画变量
    unsigned long lastAnimUpdate;
    unsigned long animStartTime;
    
    // 晴天动画
    int sunBreathPhase;
    
    // 多云动画
    int cloudFlowOffset;
    unsigned long lastCloudFlow;
    bool sunPeekOut;
    
    // 雨天动画
    static const int MAX_RAINDROPS = 6;
    Raindrop raindrops[MAX_RAINDROPS];
    unsigned long lastRainSpawn;
    
    // 雪天动画
    static const int MAX_SNOWFLAKES = 4;
    Snowflake snowflakes[MAX_SNOWFLAKES];
    unsigned long lastSnowSpawn;
    
    // 雷暴动画
    Lightning lightning;
    unsigned long lastLightning;
    unsigned int lightningInterval;
    
    // 雾天动画
    int fogFlowPhase;
    unsigned long lastFogUpdate;
    
    // 私有辅助函数
    WeatherType parseWeatherCode(const String &code);
    void updateSunnyAnimation();
    void updateCloudyAnimation();
    void updateRainAnimation();
    void updateSnowAnimation();
    void updateThunderstormAnimation();
    void updateFogAnimation();
    
    void spawnRaindrop();
    void spawnSnowflake();
    void triggerLightning();
    
    int getUmbrellaLED(int rib, int position);
    void setUmbrellaPixel(int rib, int position, int r, int g, int b);
    void setRadialRing(int position, int r, int g, int b);
    uint32_t blendColors(uint32_t color1, uint32_t color2, float ratio);
    void flushToController(); // 一次性发送所有LED数据
    
public:
    WeatherAnimation();
    void begin(LuminaireController *ctrl);
    void updateWeatherData(const String &code, int cloud, float precip, int vis);
    void update();
    void clear();
};

#endif
