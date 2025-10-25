#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>

class MQTTManager;
class LightController;
class LuminaireController;

class ButtonManager
{
private:
    static const int BUTTON_PIN = 1;           // 按钮连接到引脚1
    static const unsigned long DEBOUNCE_DELAY = 50;    // 防抖延迟 50ms
    static const unsigned long LONG_PRESS_TIME = 2000; // 长按时间 2秒

    MQTTManager *mqtt;
    LightController *localController;
    LuminaireController *luminaireController;
    int *currentMode;  // 使用 int* 代替 ControllerMode*

    bool lastButtonState;       // 上一次按钮状态
    bool buttonState;           // 当前去抖后的按钮状态
    unsigned long lastDebounceTime; // 上次状态变化时间
    unsigned long buttonPressTime;  // 按钮按下的时间
    bool buttonPressed;         // 按钮是否被按下
    bool longPressHandled;      // 长按是否已处理

    void handleShortPress();    // 处理短按（切换模式）
    void handleLongPress();     // 处理长按（开关灯）

public:
    ButtonManager();
    void begin(MQTTManager *mqttManager, 
               LightController *localCtrl, 
               LuminaireController *luminaireCtrl,
               int *mode);  // 使用 int* 代替 ControllerMode*
    void loop();
};

#endif
