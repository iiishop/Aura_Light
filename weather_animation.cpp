#include "weather_animation.h"
#include "luminaire_controller.h"

WeatherAnimation::WeatherAnimation()
    : controller(nullptr),
      weatherCode("113"),
      weatherType(WEATHER_SUNNY),
      cloudCover(0),
      precipitation(0.0),
      visibility(10),
      lastAnimUpdate(0),
      animStartTime(0),
      sunBreathPhase(0),
      cloudFlowOffset(0),
      lastCloudFlow(0),
      sunPeekOut(false),
      lastRainSpawn(0),
      lastSnowSpawn(0),
      lastLightning(0),
      lightningInterval(0),
      fogFlowPhase(0),
      lastFogUpdate(0)
{
    // 初始化本地缓存
    memset(localBuffer, 0, sizeof(localBuffer));
    
    // 初始化雨滴
    for (int i = 0; i < MAX_RAINDROPS; i++)
    {
        raindrops[i].active = false;
    }
    
    // 初始化雪花
    for (int i = 0; i < MAX_SNOWFLAKES; i++)
    {
        snowflakes[i].active = false;
    }
    
    // 初始化闪电
    lightning.active = false;
}

void WeatherAnimation::begin(LuminaireController *ctrl)
{
    controller = ctrl;
    animStartTime = millis();
}

void WeatherAnimation::updateWeatherData(const String &code, int cloud, float precip, int vis)
{
    weatherCode = code;
    cloudCover = cloud;
    precipitation = precip;
    visibility = vis;
    weatherType = parseWeatherCode(code);
}

WeatherType WeatherAnimation::parseWeatherCode(const String &code)
{
    int codeNum = code.toInt();
    
    // 晴天
    if (codeNum == 113)
        return WEATHER_SUNNY;
    
    // 多云
    if (codeNum == 116 || codeNum == 119 || codeNum == 122)
        return WEATHER_CLOUDY;
    
    // 雷暴
    if (codeNum == 200 || (codeNum >= 386 && codeNum <= 395))
        return WEATHER_THUNDERSTORM;
    
    // 雾天
    if (codeNum == 143 || codeNum == 248 || codeNum == 260)
        return WEATHER_FOG;
    
    // 雪天 (雪代码通常在300-399范围)
    if (codeNum >= 323 && codeNum <= 371)
        return WEATHER_SNOW;
    
    // 雨天 (其他降水代码)
    if (codeNum >= 176 && codeNum <= 320)
        return WEATHER_RAIN;
    
    return WEATHER_UNKNOWN;
}

void WeatherAnimation::update()
{
    if (!controller) return;
    
    unsigned long now = millis();
    
    // 清空本地缓存
    memset(localBuffer, 0, sizeof(localBuffer));
    
    // 根据天气类型更新动画
    switch (weatherType)
    {
        case WEATHER_SUNNY:
            updateSunnyAnimation();
            break;
        case WEATHER_CLOUDY:
            updateCloudyAnimation();
            break;
        case WEATHER_RAIN:
            updateRainAnimation();
            break;
        case WEATHER_SNOW:
            updateSnowAnimation();
            break;
        case WEATHER_THUNDERSTORM:
            updateThunderstormAnimation();
            break;
        case WEATHER_FOG:
            updateFogAnimation();
            break;
        default:
            break;
    }
    
    // 一次性发送所有数据
    flushToController();
}

// ============ 晴天动画 ============
void WeatherAnimation::updateSunnyAnimation()
{
    unsigned long now = millis();
    
    // 3秒周期的呼吸效果
    float breathCycle = (now % 3000) / 3000.0; // 0.0 - 1.0
    float brightness = (sin(breathCycle * 2 * PI - PI/2) + 1.0) / 2.0; // 0.0 - 1.0
    
    // LED 0: 黄色高亮脉动（太阳核心，中心）
    int coreR = 255;
    int coreG = 200 + (int)(55 * brightness);
    int coreB = 0;
    setRadialRing(0, coreR, coreG, coreB);
    
    // LED 1: 金色光束
    int beam1R = 220 - (int)(60 * (1.0 - brightness));
    int beam1G = 160 - (int)(40 * (1.0 - brightness));
    int beam1B = 0;
    setRadialRing(1, beam1R, beam1G, beam1B);
    
    // LED 2: 金色光束（更淡）
    int beam2R = 180 - (int)(60 * (1.0 - brightness));
    int beam2G = 120 - (int)(40 * (1.0 - brightness));
    int beam2B = 0;
    setRadialRing(2, beam2R, beam2G, beam2B);
    
    // 其他LED保持暗
    for (int pos = 3; pos < 6; pos++)
    {
        setRadialRing(pos, 0, 0, 0);
    }
}

// ============ 多云动画 ============
void WeatherAnimation::updateCloudyAnimation()
{
    unsigned long now = millis();
    
    // 云团流动（每500ms更新）
    if (now - lastCloudFlow > 500)
    {
        cloudFlowOffset = (cloudFlowOffset + 1) % 3;
        lastCloudFlow = now;
        
        // 每3秒决定是否露出阳光
        if ((now / 3000) % 2 == 0)
            sunPeekOut = true;
        else
            sunPeekOut = false;
    }
    
    // 计算云的厚度（基于cloudCover）
    int grayLevel = map(cloudCover, 0, 100, 80, 200);
    
    // LED 0: 中心（阳光或云）
    if (sunPeekOut)
    {
        setRadialRing(0, 255, 220, 100); // 金色阳光
    }
    else
    {
        setRadialRing(0, grayLevel, grayLevel, grayLevel); // 灰白云
    }
    
    // LED 1: 云团
    int cloud1 = grayLevel - 20 + (cloudFlowOffset == 0 ? 30 : 0);
    setRadialRing(1, cloud1, cloud1, cloud1);
    
    // LED 2: 云团
    int cloud2 = grayLevel - 40 + (cloudFlowOffset == 1 ? 30 : 0);
    setRadialRing(2, cloud2, cloud2, cloud2);
    
    // LED 3: 云团边缘
    int cloud3 = grayLevel - 60 + (cloudFlowOffset == 2 ? 30 : 0);
    setRadialRing(3, cloud3, cloud3, cloud3);
    
    // LED 4-5: 暗
    for (int pos = 4; pos < 6; pos++)
    {
        setRadialRing(pos, 0, 0, 0);
    }
}

// ============ 雨天动画 ============
void WeatherAnimation::updateRainAnimation()
{
    unsigned long now = millis();
    
    // 根据降水量决定雨滴生成频率
    int spawnInterval;
    if (precipitation < 1.0)
        spawnInterval = 800; // 小雨
    else if (precipitation < 5.0)
        spawnInterval = 400; // 中雨
    else
        spawnInterval = 200; // 大雨
    
    // 生成新雨滴
    if (now - lastRainSpawn > spawnInterval)
    {
        spawnRaindrop();
        lastRainSpawn = now;
    }
    
    // 注意：不需要清空，因为 update() 已经清空了 localBuffer
    
    // 更新并绘制雨滴
    for (int i = 0; i < MAX_RAINDROPS; i++)
    {
        if (!raindrops[i].active) continue;
        
        // 更新位置（加速下落）
        unsigned long dt = now - raindrops[i].lastUpdate;
        raindrops[i].position += raindrops[i].speed * dt / 1000.0;
        raindrops[i].speed += 3.0 * dt / 1000.0; // 重力加速
        raindrops[i].lastUpdate = now;
        
        // 检查是否到达底部（位置5是边缘）
        if (raindrops[i].position >= 5.0)
        {
            // 产生涟漪效果（LED 5闪烁）
            setRadialRing(5, 100, 150, 255);
            raindrops[i].active = false;
            continue;
        }
        
        // 绘制雨滴
        int pos = (int)raindrops[i].position;
        setUmbrellaPixel(raindrops[i].rib, pos, 50, 100, 255);
    }
}

void WeatherAnimation::spawnRaindrop()
{
    // 找到空闲的雨滴槽
    for (int i = 0; i < MAX_RAINDROPS; i++)
    {
        if (!raindrops[i].active)
        {
            raindrops[i].rib = random(0, 12);
            raindrops[i].position = 0.0;
            raindrops[i].speed = 5.0; // 初始速度
            raindrops[i].active = true;
            raindrops[i].lastUpdate = millis();
            break;
        }
    }
}

// ============ 雪天动画 ============
void WeatherAnimation::updateSnowAnimation()
{
    unsigned long now = millis();
    
    // 生成新雪花（比雨慢）
    if (now - lastSnowSpawn > 1200)
    {
        spawnSnowflake();
        lastSnowSpawn = now;
    }
    
    // 注意：不需要清空，因为 update() 已经清空了 localBuffer
    
    // 更新并绘制雪花
    for (int i = 0; i < MAX_SNOWFLAKES; i++)
    {
        if (!snowflakes[i].active) continue;
        
        // 更新位置（慢速飘落）
        unsigned long dt = now - snowflakes[i].lastUpdate;
        snowflakes[i].position += snowflakes[i].speed * dt / 1000.0;
        snowflakes[i].lastUpdate = now;
        
        // 摇摆效果（跳到相邻伞骨）
        if (random(0, 100) < 5 && snowflakes[i].position > 1.0)
        {
            snowflakes[i].rib = (snowflakes[i].rib + (random(0, 2) ? 1 : -1) + 12) % 12;
        }
        
        // 检查是否到达底部（位置5是边缘）
        if (snowflakes[i].position >= 5.0)
        {
            // 积雪效果（在LED 5停留）
            setUmbrellaPixel(snowflakes[i].rib, 5, 200, 220, 255);
            
            // 延迟失活
            if (snowflakes[i].position >= 7.0)
            {
                snowflakes[i].active = false;
            }
            continue;
        }
        
        // 绘制雪花（白色/淡蓝）
        int pos = (int)snowflakes[i].position;
        setUmbrellaPixel(snowflakes[i].rib, pos, 200, 220, 255);
    }
}

void WeatherAnimation::spawnSnowflake()
{
    for (int i = 0; i < MAX_SNOWFLAKES; i++)
    {
        if (!snowflakes[i].active)
        {
            snowflakes[i].rib = random(0, 12);
            snowflakes[i].position = 0.0;
            snowflakes[i].speed = 1.5; // 比雨慢3倍
            snowflakes[i].active = true;
            snowflakes[i].lastUpdate = millis();
            snowflakes[i].swayDirection = 0;
            break;
        }
    }
}

// ============ 雷暴动画 ============
void WeatherAnimation::updateThunderstormAnimation()
{
    unsigned long now = millis();
    
    // 触发新闪电（随机间隔2-5秒）
    if (!lightning.active && now - lastLightning > lightningInterval)
    {
        triggerLightning();
        lastLightning = now;
        lightningInterval = random(2000, 5000);
    }
    
    // 绘制背景云层（注意：update() 已经清空了 localBuffer，所以直接绘制）
    int grayLevel = 60;
    for (int pos = 0; pos < 6; pos++)
    {
        setRadialRing(pos, grayLevel, grayLevel, grayLevel);
    }
    
    // 更新闪电动画
    if (lightning.active)
    {
        unsigned long elapsed = now - lightning.startTime;
        
        if (elapsed < 50) // 闪电持续50ms
        {
            lightning.progress = elapsed / 50.0;
            
            // 闪电从LED 0冲向LED 5（中心到边缘）
            int lightningPos = (int)(lightning.progress * 5);
            
            for (int i = 0; i < lightning.ribCount; i++)
            {
                for (int pos = 0; pos <= lightningPos; pos++)
                {
                    setUmbrellaPixel(lightning.ribs[i], pos, 255, 255, 255);
                }
            }
            
            // LED 5（边缘）全体爆闪
            if (lightningPos >= 4)
            {
                setRadialRing(5, 255, 255, 255);
            }
        }
        else
        {
            lightning.active = false;
        }
    }
}

void WeatherAnimation::triggerLightning()
{
    lightning.ribCount = random(1, 3); // 1-2条闪电
    lightning.ribs[0] = random(0, 12);
    if (lightning.ribCount == 2)
    {
        lightning.ribs[1] = (lightning.ribs[0] + random(2, 6)) % 12;
    }
    lightning.progress = 0.0;
    lightning.active = true;
    lightning.startTime = millis();
}

// ============ 雾天动画 ============
void WeatherAnimation::updateFogAnimation()
{
    unsigned long now = millis();
    
    // 雾气流动（每100ms更新）
    if (now - lastFogUpdate > 100)
    {
        fogFlowPhase = (fogFlowPhase + 1) % 360;
        lastFogUpdate = now;
    }
    
    // 基于能见度决定雾的浓度
    int fogDensity = map(visibility, 0, 10, 150, 50);
    
    // 所有LED朦胧效果（上下流动）
    for (int pos = 0; pos < 6; pos++)
    {
        float wave = sin((fogFlowPhase + pos * 30) * PI / 180.0);
        int brightness = fogDensity + (int)(20 * wave);
        brightness = constrain(brightness, 30, 200);
        
        setRadialRing(pos, brightness, brightness, brightness);
    }
}

// ============ 工具函数 ============
int WeatherAnimation::getUmbrellaLED(int rib, int position)
{
    if (rib < 0 || rib >= 12 || position < 0 || position >= 6)
        return -1;
    return rib * 6 + position;
}

void WeatherAnimation::setUmbrellaPixel(int rib, int position, int r, int g, int b)
{
    int ledNum = getUmbrellaLED(rib, position);
    if (ledNum >= 0 && ledNum < 72)
    {
        localBuffer[ledNum * 3 + 0] = (byte)r;
        localBuffer[ledNum * 3 + 1] = (byte)g;
        localBuffer[ledNum * 3 + 2] = (byte)b;
    }
}

void WeatherAnimation::setRadialRing(int position, int r, int g, int b)
{
    for (int rib = 0; rib < 12; rib++)
    {
        setUmbrellaPixel(rib, position, r, g, b);
    }
}

void WeatherAnimation::flushToController()
{
    if (!controller) return;
    
    // 一次性发送所有72个LED的数据
    controller->updateAllLEDs(localBuffer, 72 * 3);
    
    // 调试：每5秒打印一次动画状态
    static unsigned long lastDebug = 0;
    unsigned long now = millis();
    if (now - lastDebug > 5000)
    {
        Serial.print("[Weather Anim] Type: ");
        Serial.print(weatherType);
        Serial.print(", Code: ");
        Serial.println(weatherCode);
        lastDebug = now;
    }
}

void WeatherAnimation::clear()
{
    if (!controller) return;
    controller->clear();
}

uint32_t WeatherAnimation::blendColors(uint32_t color1, uint32_t color2, float ratio)
{
    uint8_t r1 = (color1 >> 16) & 0xFF;
    uint8_t g1 = (color1 >> 8) & 0xFF;
    uint8_t b1 = color1 & 0xFF;
    
    uint8_t r2 = (color2 >> 16) & 0xFF;
    uint8_t g2 = (color2 >> 8) & 0xFF;
    uint8_t b2 = color2 & 0xFF;
    
    uint8_t r = r1 + (r2 - r1) * ratio;
    uint8_t g = g1 + (g2 - g1) * ratio;
    uint8_t b = b1 + (b2 - b1) * ratio;
    
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}
