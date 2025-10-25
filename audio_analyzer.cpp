#include "audio_analyzer.h"

AudioAnalyzer::AudioAnalyzer()
    : minDecibel(MIN_DB),
      maxDecibel(MAX_DB),
      lastSampleTime(0),
      currentVolume(0.0),
      smoothedVolume(0.0),
      lastRawADC(0),
      lastBandUpdate(0)
{
    for (int i = 0; i < 12; i++)
    {
        virtualBands[i] = 0.0;
    }
}

void AudioAnalyzer::begin()
{
    pinMode(AUDIO_PIN, INPUT);

    Serial.println("[AudioAnalyzer] Initialized");
    Serial.print("[AudioAnalyzer] Input pin: A");
    Serial.println(AUDIO_PIN - A0);
    Serial.print("[AudioAnalyzer] Volume range: ");
    Serial.print(minDecibel);
    Serial.print(" - ");
    Serial.print(maxDecibel);
    Serial.println(" dB");
}

void AudioAnalyzer::loop()
{
    unsigned long currentTime = millis();

    // 每 SAMPLE_WINDOW ms 采样一次
    if (currentTime - lastSampleTime >= SAMPLE_WINDOW)
    {
        lastSampleTime = currentTime;

        // 读取音量
        float rawVol = readRawVolume();
        currentVolume = rawVol;

        // 平滑处理
        smoothedVolume = smoothedVolume * (1.0 - SMOOTHING_FACTOR) + rawVol * SMOOTHING_FACTOR;

        // 限制范围
        if (smoothedVolume < 0.0)
            smoothedVolume = 0.0;
        if (smoothedVolume > 1.0)
            smoothedVolume = 1.0;
    }

    // 每 20ms 更新一次虚拟频段
    if (currentTime - lastBandUpdate >= 20)
    {
        lastBandUpdate = currentTime;
        updateVirtualBands();
    }
}

float AudioAnalyzer::readRawVolume()
{
    unsigned long startMillis = millis();
    unsigned int peakToPeak = 0;
    unsigned int signalMax = 0;
    unsigned int signalMin = 1024;
    unsigned int sampleCount = 0;
    unsigned long sampleSum = 0;

    // 在 SAMPLE_WINDOW 内收集数据
    while (millis() - startMillis < SAMPLE_WINDOW)
    {
        unsigned int sample = analogRead(AUDIO_PIN);

        if (sample < 1024)
        {
            if (sample > signalMax)
            {
                signalMax = sample;
            }
            if (sample < signalMin)
            {
                signalMin = sample;
            }

            // 累加样本用于计算平均值
            sampleSum += sample;
            sampleCount++;
        }
    }

    peakToPeak = signalMax - signalMin;

    // 浮空检测：如果峰峰值太小（< 5 ADC counts），可能是浮空或无信号
    // 如果平均值接近 0 或 1023，也可能是接触不良
    if (sampleCount > 0)
    {
        unsigned int avgValue = sampleSum / sampleCount;

        // 诊断信息（可选，调试时启用）
        static unsigned long lastDiagnostic = 0;
        if (millis() - lastDiagnostic > 5000) // 每 5 秒打印一次
        {
            Serial.print("[AudioAnalyzer] Peak-to-peak: ");
            Serial.print(peakToPeak);
            Serial.print(", Avg: ");
            Serial.print(avgValue);
            Serial.print(", Min: ");
            Serial.print(signalMin);
            Serial.print(", Max: ");
            Serial.println(signalMax);

            if (peakToPeak < 5)
            {
                Serial.println("[AudioAnalyzer] ⚠️ WARNING: Very low signal - check MAX9814 connection!");
            }
            if (avgValue < 10 || avgValue > 1014)
            {
                Serial.println("[AudioAnalyzer] ⚠️ WARNING: Floating pin detected - OUT pin may be disconnected!");
            }

            lastDiagnostic = millis();
        }

        // 如果峰峰值小于噪声阈值（5），强制返回 0
        if (peakToPeak < 5)
        {
            lastRawADC = 0;
            return 0.0;
        }
    }

    // 保存原始 ADC 值用于 Dashboard 显示
    lastRawADC = peakToPeak;

    // 转换为 0.0 - 1.0
    // MAX9814 输出范围是 0-VDD，对应 ADC 0-1023
    float voltage = (peakToPeak * 3.3) / 1024.0;

    // 简化的音量计算（0-3.3V 映射到 0.0-1.0）
    float volume = voltage / 3.3;

    return volume;
}

float AudioAnalyzer::volumeToDecibel(float vol) const
{
    if (vol <= 0.0)
        return minDecibel;

    // 简化的 dB 转换：使用对数刻度
    // 0.0 -> minDecibel, 1.0 -> maxDecibel
    float db = minDecibel + (maxDecibel - minDecibel) * vol;

    return db;
}

void AudioAnalyzer::updateVirtualBands()
{
    // 创建虚拟频谱效果
    // 基于音量变化速度和总音量来模拟不同频段

    float deltaVolume = abs(currentVolume - smoothedVolume);

    for (int i = 0; i < 12; i++)
    {
        float targetLevel;

        if (i < 3)
        {
            // 低频段（0-2）：跟随总音量，变化较慢
            targetLevel = smoothedVolume * 0.9;
        }
        else if (i < 7)
        {
            // 中频段（3-6）：主要音量 + 轻微随机
            targetLevel = smoothedVolume * 0.95 + deltaVolume * 2.0;
        }
        else
        {
            // 高频段（7-11）：快速变化，跟随瞬时音量
            targetLevel = currentVolume * 0.8 + deltaVolume * 5.0;
        }

        // 添加轻微随机性使效果更自然
        float randomFactor = (random(80, 120) / 100.0);
        targetLevel *= randomFactor;

        // 限制范围
        if (targetLevel > 1.0)
            targetLevel = 1.0;
        if (targetLevel < 0.0)
            targetLevel = 0.0;

        // 平滑过渡
        float bandSmoothing = 0.4;
        virtualBands[i] = virtualBands[i] * (1.0 - bandSmoothing) + targetLevel * bandSmoothing;

        // 衰减效果
        virtualBands[i] *= 0.95;
    }
}

void AudioAnalyzer::setVolumeRange(float minDb, float maxDb)
{
    if (minDb >= maxDb || minDb < 0 || maxDb > 130)
    {
        Serial.println("[AudioAnalyzer] ⚠️ Invalid volume range!");
        return;
    }

    minDecibel = minDb;
    maxDecibel = maxDb;

    Serial.print("[AudioAnalyzer] Volume range updated: ");
    Serial.print(minDecibel);
    Serial.print(" - ");
    Serial.print(maxDecibel);
    Serial.println(" dB");
}

void AudioAnalyzer::getVolumeRange(float &minDb, float &maxDb) const
{
    minDb = minDecibel;
    maxDb = maxDecibel;
}

float AudioAnalyzer::getVolumeDecibel() const
{
    return volumeToDecibel(smoothedVolume);
}

int AudioAnalyzer::getVolumeLevel(int maxLevels) const
{
    if (maxLevels <= 0)
        return 0;

    int level = (int)(smoothedVolume * maxLevels);

    if (level >= maxLevels)
        level = maxLevels - 1;
    if (level < 0)
        level = 0;

    // 调试：定期打印音量映射
    static unsigned long lastVUDebug = 0;
    if (millis() - lastVUDebug > 3000)
    {
        Serial.print("[AudioAnalyzer] smoothedVolume: ");
        Serial.print(smoothedVolume, 3);
        Serial.print(" → Level: ");
        Serial.print(level);
        Serial.print(" / ");
        Serial.println(maxLevels - 1);
        lastVUDebug = millis();
    }

    return level;
}

void AudioAnalyzer::getVirtualBands(float bands[12]) const
{
    for (int i = 0; i < 12; i++)
    {
        bands[i] = virtualBands[i];
    }
}

float AudioAnalyzer::getVirtualBand(int index) const
{
    if (index < 0 || index >= 12)
        return 0.0;
    return virtualBands[index];
}
