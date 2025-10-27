#include "audio_analyzer.h"

AudioAnalyzer::AudioAnalyzer()
    : minDecibel(MIN_DB),
      maxDecibel(MAX_DB),
      lastFFTTime(0),
      currentVolume(0.0),
      smoothedVolume(0.0),
      lastRawADC(0),
      FFT(ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY)) // 创建 FFT 对象
{
    // 初始化数组
    for (int i = 0; i < NUM_BANDS; i++)
    {
        spectrumBands[i] = 0.0;
        smoothedBands[i] = 0.0;
    }

    for (int i = 0; i < SAMPLES; i++)
    {
        vReal[i] = 0.0;
        vImag[i] = 0.0;
    }

    // 计算采样周期（微秒）
    sampling_period_us = round(1000000.0 / SAMPLING_FREQUENCY);
}

void AudioAnalyzer::begin()
{
    pinMode(AUDIO_PIN, INPUT);

    Serial.println("[AudioAnalyzer] FFT-based analyzer initialized");
    Serial.print("[AudioAnalyzer] Input pin: A");
    Serial.println(AUDIO_PIN - A0);
    Serial.print("[AudioAnalyzer] Sampling frequency: ");
    Serial.print(SAMPLING_FREQUENCY);
    Serial.println(" Hz");
    Serial.print("[AudioAnalyzer] Samples: ");
    Serial.println(SAMPLES);
    Serial.print("[AudioAnalyzer] Frequency resolution: ");
    Serial.print(SAMPLING_FREQUENCY / SAMPLES);
    Serial.println(" Hz/bin");
    Serial.print("[AudioAnalyzer] Number of bands: ");
    Serial.println(NUM_BANDS);
    Serial.print("[AudioAnalyzer] Sampling period: ");
    Serial.print(sampling_period_us);
    Serial.println(" us");
}

void AudioAnalyzer::loop()
{
    unsigned long currentTime = millis();

    // 每 30-50ms 执行一次 FFT（避免过于频繁）
    if (currentTime - lastFFTTime >= 40)
    {
        lastFFTTime = currentTime;
        performFFT();
        updateBands();
        currentVolume = calculateVolume();

        // 平滑总音量
        smoothedVolume = smoothedVolume * 0.7 + currentVolume * 0.3;
    }
}

void AudioAnalyzer::performFFT()
{
    // 采样音频数据（参考 LEDSpectrum 项目的方法）
    unsigned long microseconds = micros();
    unsigned int signalMax = 0;
    unsigned int signalMin = 1024;

    for (int i = 0; i < SAMPLES; i++)
    {
        unsigned int sample = analogRead(AUDIO_PIN);
        vReal[i] = sample; // 直接存储 ADC 值
        vImag[i] = 0.0;    // 虚部清零

        // 跟踪峰峰值用于总音量计算
        if (sample > signalMax)
            signalMax = sample;
        if (sample < signalMin)
            signalMin = sample;

        // 精确等待下一个采样周期
        while (micros() - microseconds < sampling_period_us)
        {
            // 空循环等待
        }
        microseconds += sampling_period_us;
    }

    // 保存峰峰值
    lastRawADC = signalMax - signalMin;

    // 执行 FFT（使用 arduinoFFT 2.x 新版 API）
    FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward); // 汉明窗
    FFT.compute(FFTDirection::Forward);                       // 计算 FFT
    FFT.complexToMagnitude();                                 // 转换为幅度谱
}

void AudioAnalyzer::updateBands()
{
    // 简化的频段映射方法（参考 LEDSpectrum）
    // 4000 Hz / 64 samples = 62.5 Hz/bin
    // FFT 输出 32 个有效 bins (0-2000 Hz)
    //
    // 12 频段直接从 FFT bins 选取（跳过 bin 0 和 1 是 DC 和极低频噪声）:
    // Band 0:  bin 2-3    (125-187 Hz)  - 低音
    // Band 1:  bin 4-5    (250-312 Hz)  - 低音
    // Band 2:  bin 6-7    (375-437 Hz)  - 中低音
    // Band 3:  bin 8-9    (500-562 Hz)  - 中低音
    // Band 4:  bin 10-11  (625-687 Hz)  - 中音
    // Band 5:  bin 12-13  (750-812 Hz)  - 中音
    // Band 6:  bin 14-15  (875-937 Hz)  - 中高音
    // Band 7:  bin 16-17  (1000-1062 Hz) - 中高音
    // Band 8:  bin 18-20  (1125-1250 Hz) - 高音
    // Band 9:  bin 21-23  (1312-1437 Hz) - 高音
    // Band 10: bin 24-26  (1500-1625 Hz) - 极高音
    // Band 11: bin 27-30  (1687-1875 Hz) - 极高音

    // 直接取平均值的方法
    spectrumBands[0] = (vReal[2] + vReal[3]) / 2.0;
    spectrumBands[1] = (vReal[4] + vReal[5]) / 2.0;
    spectrumBands[2] = (vReal[6] + vReal[7]) / 2.0;
    spectrumBands[3] = (vReal[8] + vReal[9]) / 2.0;
    spectrumBands[4] = (vReal[10] + vReal[11]) / 2.0;
    spectrumBands[5] = (vReal[12] + vReal[13]) / 2.0;
    spectrumBands[6] = (vReal[14] + vReal[15]) / 2.0;
    spectrumBands[7] = (vReal[16] + vReal[17]) / 2.0;
    spectrumBands[8] = (vReal[18] + vReal[19] + vReal[20]) / 3.0;
    spectrumBands[9] = (vReal[21] + vReal[22] + vReal[23]) / 3.0;
    spectrumBands[10] = (vReal[24] + vReal[25] + vReal[26]) / 3.0;
    spectrumBands[11] = (vReal[27] + vReal[28] + vReal[29] + vReal[30]) / 4.0;

    // 找到最大值用于归一化
    float maxMagnitude = 0.0;
    for (int i = 0; i < NUM_BANDS; i++)
    {
        if (spectrumBands[i] > maxMagnitude)
            maxMagnitude = spectrumBands[i];
    }

    // 归一化到 0.0-1.0 并平滑处理
    if (maxMagnitude > 10.0) // 避免除以零或过小的值
    {
        for (int i = 0; i < NUM_BANDS; i++)
        {
            // 归一化
            spectrumBands[i] = spectrumBands[i] / maxMagnitude;

            // 平滑处理（60% 旧值 + 40% 新值）
            smoothedBands[i] = smoothedBands[i] * 0.6 + spectrumBands[i] * 0.4;

            // 限制范围
            if (smoothedBands[i] < 0.0)
                smoothedBands[i] = 0.0;
            if (smoothedBands[i] > 1.0)
                smoothedBands[i] = 1.0;
        }
    }

    // 调试输出（每 2 秒）
    static unsigned long lastDebug = 0;
    if (millis() - lastDebug > 2000)
    {
        Serial.print("[FFT Bands] ");
        for (int i = 0; i < NUM_BANDS; i++)
        {
            Serial.print(i);
            Serial.print(":");
            Serial.print(smoothedBands[i], 2);
            Serial.print(" ");
        }
        Serial.print(" | Max:");
        Serial.print(maxMagnitude, 1);
        Serial.println();
        lastDebug = millis();
    }
}

float AudioAnalyzer::calculateVolume()
{
    // 从 FFT 结果计算总能量（RMS）
    double totalEnergy = 0.0;
    int count = 0;

    // 使用 FFT bin 2 到 SAMPLES/2-1（跳过直流和最高频）
    for (int i = 2; i < SAMPLES / 2; i++)
    {
        totalEnergy += vReal[i] * vReal[i];
        count++;
    }

    if (count > 0)
    {
        totalEnergy = sqrt(totalEnergy / count);
    }

    // 归一化到 0.0-1.0
    // ADC 范围 0-1023，RMS 大约在 0-500 范围
    float volume = totalEnergy / 500.0;

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
    // 参考点 1: 0.17V → 30 dB (安静)
    // 参考点 2: 2.50V → 65 dB (大声)

    const float voltage_quiet = 0.17; // 安静环境参考电压
    const float voltage_loud = 2.50;  // 大声环境参考电压
    const float db_quiet = 30.0;      // 对应的分贝值（降低）
    const float db_loud = 65.0;       // 对应的分贝值（降低）

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

    return absoluteDb;
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

    return level;
}

void AudioAnalyzer::getVirtualBands(float bands[NUM_BANDS]) const
{
    // 返回真实的 FFT 频段数据
    for (int i = 0; i < NUM_BANDS; i++)
    {
        bands[i] = smoothedBands[i];
    }
}

float AudioAnalyzer::getVirtualBand(int index) const
{
    if (index < 0 || index >= NUM_BANDS)
        return 0.0;
    return smoothedBands[index];
}
