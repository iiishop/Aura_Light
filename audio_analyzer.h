#ifndef AUDIO_ANALYZER_H
#define AUDIO_ANALYZER_H

#include <Arduino.h>

#define AUDIO_PIN A0         // MAX9814 连接到 A0
#define SAMPLE_WINDOW 50     // 采样窗口 50ms
#define MIN_DB 30.0          // 最小音量（默认）
#define MAX_DB 120.0         // 最大音量（默认）
#define SMOOTHING_FACTOR 0.3 // 平滑系数（0-1，越小越平滑）

class AudioAnalyzer
{
private:
    // 配置参数
    float minDecibel; // 最小音量阈值（dB）
    float maxDecibel; // 最大音量阈值（dB）

    // 采样数据
    unsigned long lastSampleTime;
    float currentVolume;  // 当前音量（0.0 - 1.0）
    float smoothedVolume; // 平滑后的音量
    int lastRawADC;       // 最后一次原始 ADC 值（0-1023）

    // 虚拟频谱数据（12 频段）
    float virtualBands[12]; // 12 个虚拟频段的强度（0.0 - 1.0）
    unsigned long lastBandUpdate;

    // 私有方法
    float readRawVolume();                  // 读取原始音量
    float volumeToDecibel(float vol) const; // 转换为分贝
    void updateVirtualBands();              // 更新虚拟频段

public:
    AudioAnalyzer();

    void begin();
    void loop();

    // 配置
    void setVolumeRange(float minDb, float maxDb);
    void getVolumeRange(float &minDb, float &maxDb) const;

    // 获取音量数据
    int getRawADC() const { return lastRawADC; } // 原始 ADC 值（0-1023）
    float getVolume() const;                     // 基于用户设置范围的归一化音量（0.0 - 1.0）
    float getVolumeDecibel() const;              // 真实的绝对分贝值
    int getVolumeLevel(int maxLevels) const;     // 离散级别（如 0-7）

    // 获取虚拟频段数据（用于 Luminaire）
    void getVirtualBands(float bands[12]) const;
    float getVirtualBand(int index) const;
};

#endif
