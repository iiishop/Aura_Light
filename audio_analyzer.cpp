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
                Serial.println("[AudioAnalyzer] WARNING: Very low signal - check MAX9814 connection!");
            }
            if (avgValue < 10 || avgValue > 1014)
            {
                Serial.println("[AudioAnalyzer] WARNING: Floating pin detected - OUT pin may be disconnected!");
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
    // 基于实际硬件测试的优化校准
    // 实测数据:
    // - 安静环境: voltage ≈ 0.17V (vol ≈ 0.05) → 约 35 dB
    // - 大声说话: voltage ≈ 2.48V (vol ≈ 0.75) → 约 80 dB
    // - 硬件限制: 最大峰峰值 ≈ 774 (约 2.5V)

    // vol 的范围是 0.0-1.0，对应峰峰值电压 0-3.3V
    float voltage = vol * 3.3;

    // 极低电压认为是噪声
    if (voltage < 0.005)
    {
        return 20.0;
    }

    // 优化的两点线性插值法
    // 参考点 1: 0.17V → 35 dB (安静)
    // 参考点 2: 2.50V → 80 dB (大声)

    const float voltage_quiet = 0.17; // 安静环境参考电压
    const float voltage_loud = 2.50;  // 大声环境参考电压
    const float db_quiet = 35.0;      // 对应的分贝值
    const float db_loud = 80.0;       // 对应的分贝值

    float absoluteDb;

    if (voltage <= voltage_quiet)
    {
        // 低于安静参考点，使用对数外推
        float relativeDb = 20.0 * log10(voltage / voltage_quiet);
        absoluteDb = db_quiet + relativeDb;
    }
    else if (voltage >= voltage_loud)
    {
        // 高于大声参考点，轻微外推（避免过度）
        float relativeDb = 20.0 * log10(voltage / voltage_loud);
        absoluteDb = db_loud + relativeDb * 0.5; // 减半避免过度
    }
    else
    {
        // 在两个参考点之间，使用对数插值
        float relativeDb = 20.0 * log10(voltage / voltage_quiet);
        float maxRelativeDb = 20.0 * log10(voltage_loud / voltage_quiet);
        // 线性映射到目标范围
        absoluteDb = db_quiet + (db_loud - db_quiet) * (relativeDb / maxRelativeDb);
    }

    // 限制在合理范围内
    if (absoluteDb < 20)
        absoluteDb = 20;
    if (absoluteDb > 120)
        absoluteDb = 120;

    // 调试输出（每 3 秒）
    static unsigned long lastDebug = 0;
    if (millis() - lastDebug > 3000)
    {
        Serial.print("[AudioAnalyzer] V: ");
        Serial.print(voltage, 3);
        Serial.print("V, dB: ");
        Serial.print(absoluteDb, 1);
        Serial.print(" (range: ");
        Serial.print(db_quiet, 0);
        Serial.print("-");
        Serial.print(db_loud, 0);
        Serial.println(")");
        lastDebug = millis();
    }

    return absoluteDb;
}

void AudioAnalyzer::updateVirtualBands()
{
    // 改进的虚拟频谱算法
    // 基于音量变化的速度（一阶导数）和加速度（二阶导数）来模拟频率响应

    static float lastVolume = 0.0;
    static float lastDelta = 0.0;

    // 一阶导数：音量变化速度
    float deltaVolume = currentVolume - lastVolume;
    float absChange = abs(deltaVolume);

    // 二阶导数：音量变化的加速度（变化率的变化）
    float acceleration = deltaVolume - lastDelta;
    float absAccel = abs(acceleration);

    // 更新历史值
    lastVolume = currentVolume;
    lastDelta = deltaVolume;

    for (int i = 0; i < 12; i++)
    {
        float targetLevel;

        if (i < 2)
        {
            // 超低频段（0-1）：20-60 Hz
            // 主要跟随总音量，几乎不受快速变化影响
            targetLevel = smoothedVolume * 0.85;
            // 超低频在自然声音中较少，添加衰减
            targetLevel *= 0.7;
        }
        else if (i < 4)
        {
            // 低频段（2-3）：60-250 Hz
            // 跟随平滑音量，轻微受变化速度影响
            targetLevel = smoothedVolume * 0.90 + absChange * 1.5;
        }
        else if (i < 7)
        {
            // 中低频段（4-6）：250-1000 Hz
            // 人声和大部分乐器的主要频段
            // 平衡音量和变化速度
            targetLevel = smoothedVolume * 0.95 + absChange * 2.5;
            targetLevel *= 1.1; // 轻微增强，因为是主要频段
        }
        else if (i < 9)
        {
            // 中高频段（7-8）：1-4 kHz
            // 更多受快速变化影响
            targetLevel = currentVolume * 0.75 + absChange * 4.0 + absAccel * 2.0;
        }
        else
        {
            // 高频段（9-11）：4-8 kHz
            // 主要跟随瞬时变化和加速度
            targetLevel = currentVolume * 0.60 + absChange * 6.0 + absAccel * 4.0;
            // 高频在环境音中较少，需要明显变化才显示
            targetLevel *= 0.8;
        }

        // 添加频段特定的随机性（模拟噪声和泛音）
        float randomFactor;
        if (i < 4)
        {
            // 低频：稳定，随机性小
            randomFactor = random(90, 110) / 100.0;
        }
        else if (i < 7)
        {
            // 中频：中等随机性
            randomFactor = random(85, 115) / 100.0;
        }
        else
        {
            // 高频：不稳定，随机性大
            randomFactor = random(75, 125) / 100.0;
        }

        targetLevel *= randomFactor;

        // 限制范围
        targetLevel = constrain(targetLevel, 0.0, 1.0);

        // 频段特定的平滑系数
        float bandSmoothing;
        if (i < 3)
        {
            // 低频：慢速响应
            bandSmoothing = 0.25;
        }
        else if (i < 7)
        {
            // 中频：中速响应
            bandSmoothing = 0.40;
        }
        else
        {
            // 高频：快速响应
            bandSmoothing = 0.60;
        }

        // 平滑过渡到目标值
        virtualBands[i] = virtualBands[i] * (1.0 - bandSmoothing) + targetLevel * bandSmoothing;

        // 自然衰减（模拟声音的自然衰减特性）
        virtualBands[i] *= 0.95;
    }
}

void AudioAnalyzer::setVolumeRange(float minDb, float maxDb)
{
    if (minDb >= maxDb || minDb < 0 || maxDb > 130)
    {
        Serial.println("[AudioAnalyzer] WARNING: Invalid volume range!");
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

float AudioAnalyzer::getVolume() const
{
    // 获取真实的分贝值
    float db = getVolumeDecibel();

    // 基于用户设置的范围，将分贝值归一化到 0.0-1.0
    // 例如：范围 30-120 dB
    //   - 30 dB → 0.0
    //   - 75 dB → 0.5
    //   - 120 dB → 1.0
    float normalized = (db - minDecibel) / (maxDecibel - minDecibel);

    // 限制在 0.0-1.0 范围内
    if (normalized < 0.0)
        normalized = 0.0;
    if (normalized > 1.0)
        normalized = 1.0;

    return normalized;
}

int AudioAnalyzer::getVolumeLevel(int maxLevels) const
{
    if (maxLevels <= 0)
        return 0;

    // 使用归一化的音量（基于用户设置的范围）
    float normalizedVolume = getVolume();
    int level = (int)(normalizedVolume * maxLevels);

    if (level >= maxLevels)
        level = maxLevels - 1;
    if (level < 0)
        level = 0;

    // 调试：定期打印音量映射
    static unsigned long lastVUDebug = 0;
    if (millis() - lastVUDebug > 3000)
    {
        Serial.print("[AudioAnalyzer] dB: ");
        Serial.print(getVolumeDecibel(), 1);
        Serial.print(" → Normalized: ");
        Serial.print(normalizedVolume, 3);
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
