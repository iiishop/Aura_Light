#ifndef LUMINAIRE_CONTROLLER_H
#define LUMINAIRE_CONTROLLER_H

#include <Arduino.h>
#include "mqtt_manager.h"

// 前向声明
class MusicMode;
class AudioAnalyzer;

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

    String lightId;
    String mqttTopic;
    byte RGBpayload[LUMINAIRE_PAYLOAD_SIZE];

    bool isActive;
    LuminaireState state;
    LuminaireMode mode;

    void applyModeColor();
    void updateMusicSpectrum(); // 新增：更新 Music 频谱显示
    void getRGBFromHex(const String &hexColor, int &r, int &g, int &b);

public:
    LuminaireController();
    void begin(MQTTManager *mqttManager, const String &id);
    void setActive(bool active);
    bool getActive() const { return isActive; }

    // 设置 Music 模式和音频分析器
    void setMusicMode(MusicMode *music, AudioAnalyzer *audio);

    // 主循环（用于 Music 模式更新）
    void loop();

    void handleMQTTMessage(char *topic, byte *payload, unsigned int length);

    void sendRGBToPixel(int r, int g, int b, int pixel);

    void sendRGBToAll(int r, int g, int b);

    void clear();

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
};

#endif
