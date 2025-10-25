#include "music_mode.h"

MusicMode::MusicMode() : isActive(false), audioAnalyzer(nullptr)
{
}

void MusicMode::begin(AudioAnalyzer *analyzer)
{
    audioAnalyzer = analyzer;
    Serial.println("[MusicMode] Initialized with audio analyzer");
}

void MusicMode::setActive(bool active)
{
    isActive = active;
    if (active)
    {
        Serial.println("[MusicMode] Activated - Audio visualization mode");
    }
    else
    {
        Serial.println("[MusicMode] Deactivated");
    }
}

void MusicMode::getRGB(int &r, int &g, int &b)
{
    // Music 模式基础颜色：白色
    r = 255;
    g = 255;
    b = 255;
}

int MusicMode::getVULevel() const
{
    if (!audioAnalyzer)
    {
        return 0;
    }

    // 获取 0-7 的 VU 级别（对应 8 个 NeoPixel）
    return audioAnalyzer->getVolumeLevel(8);
}

void MusicMode::getSpectrumData(float bands[12]) const
{
    if (!audioAnalyzer)
    {
        for (int i = 0; i < 12; i++)
        {
            bands[i] = 0.0;
        }
        return;
    }

    // 获取 12 个虚拟频段的数据
    audioAnalyzer->getVirtualBands(bands);
}

void MusicMode::loop()
{
    // Music 模式的主循环
    // 实际的音频处理在 AudioAnalyzer::loop() 中
}
