#ifndef MUSIC_MODE_H
#define MUSIC_MODE_H

#include <Arduino.h>
#include "audio_analyzer.h"

class MusicMode
{
private:
    bool isActive;
    AudioAnalyzer *audioAnalyzer;

public:
    MusicMode();

    void begin(AudioAnalyzer *analyzer);

    void setActive(bool active);
    bool getActive() const { return isActive; }

    void getRGB(int &r, int &g, int &b);

    // 获取 VU 表级别（用于 Local 模式，8 个 NeoPixel）
    int getVULevel() const;

    // 获取虚拟频谱数据（用于 Luminaire 模式，12x6 网格）
    void getSpectrumData(float bands[12]) const;

    void loop();
};

#endif
