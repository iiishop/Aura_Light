#ifndef AUDIO_ANALYZER_H
#define AUDIO_ANALYZER_H

#include <Arduino.h>
#include <arduinoFFT.h>

#define AUDIO_PIN A0 // MAX9814 连接到 A0
#define MIN_DB 30.0  // 最小音量（默认）
#define MAX_DB 120.0 // 最大音量（默认）

// FFT 配置参数（参考 LEDSpectrum 项目）
#define SAMPLES 64              // 采样点数，必须为 2 的整数次幂
#define SAMPLING_FREQUENCY 4000 // 采样频率 4000 Hz（参考项目使用）
#define NUM_BANDS 12            // 频段数量

class AudioAnalyzer
{
private:
    // 配置参数
    float minDecibel; // 最小音量阈值（dB）
    float maxDecibel; // 最大音量阈值（dB）

    // FFT 相关
    ArduinoFFT<double> FFT;          // FFT 对象（arduinoFFT 2.x 新版 API）
    double vReal[SAMPLES];           // FFT 实部输入/输出
    double vImag[SAMPLES];           // FFT 虚部输入/输出
    unsigned int sampling_period_us; // 采样周期（微秒）
    unsigned long lastFFTTime;       // 上次 FFT 计算时间

    // 频段数据（12 频段）
    float spectrumBands[NUM_BANDS]; // 真实 FFT 频段强度（0.0 - 1.0）
    float smoothedBands[NUM_BANDS]; // 平滑后的频段强度

    // 音量数据
    float currentVolume;  // 当前音量（0.0 - 1.0）
    float smoothedVolume; // 平滑后的音量
    int lastRawADC;       // 最后一次原始 ADC 峰峰值

    // 私有方法
    void performFFT();                      // 执行 FFT 分析
    void updateBands();                     // 更新频段数据
    float calculateVolume();                // 从 FFT 结果计算总音量
    float volumeToDecibel(float vol) const; // 转换为分贝

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

    // 获取频段数据（用于 Luminaire）
    void getVirtualBands(float bands[NUM_BANDS]) const;
    float getVirtualBand(int index) const;
};

#endif
