#include "button_manager.h"
#include "mqtt_manager.h"
#include "light_controller.h"
#include "luminaire_controller.h"

// 控制器模式常量（对应主程序中的 ControllerMode 枚举）
#define MODE_LOCAL 0
#define MODE_LUMINAIRE 1

ButtonManager::ButtonManager()
    : mqtt(nullptr),
      localController(nullptr),
      luminaireController(nullptr),
      currentMode(nullptr),
      lastButtonState(LOW), // NC按钮初始状态为LOW（释放）
      buttonState(LOW),     // NC按钮初始状态为LOW（释放）
      lastDebounceTime(0),
      buttonPressTime(0),
      buttonPressed(false),
      longPressHandled(false)
{
}

void ButtonManager::begin(MQTTManager *mqttManager,
                          LightController *localCtrl,
                          LuminaireController *luminaireCtrl,
                          int *mode)
{
    mqtt = mqttManager;
    localController = localCtrl;
    luminaireController = luminaireCtrl;
    currentMode = mode;

    // 配置按钮引脚
    // 注意：这是常闭按钮（NC），逻辑反转：
    // - 未按时：LOW (导通到GND)
    // - 按下时：HIGH (断开)
    pinMode(BUTTON_PIN, INPUT); // 使用外部上拉（按钮模块自带）

    // 等待引脚稳定
    delay(100);

    // 读取初始状态
    int initialState = digitalRead(BUTTON_PIN);
    Serial.print("[Button] Initial pin state: ");
    Serial.println(initialState == LOW ? "LOW (released - NC button) ✓" : "HIGH (pressed - NC button)");

    Serial.println("[Button] Button manager initialized on pin 1");
    Serial.println("[Button] Button type: Normally Closed (NC)");
    Serial.println("[Button] Current wiring configuration:");
    Serial.println("[Button]   VCC → 3.3V");
    Serial.println("[Button]   OUT → Pin 1");
    Serial.println("[Button]   GND → GND");
    Serial.println("[Button] - Short press: Cycle through modes (Timer → Weather → Idle → Music)");
    Serial.println("[Button] - Long press (2s): Toggle light ON/OFF");
}

void ButtonManager::loop()
{
    // 读取当前按钮状态
    // 注意：常闭按钮（NC）逻辑反转
    // LOW = 释放（导通到GND）, HIGH = 按下（断开）
    int reading = digitalRead(BUTTON_PIN);

    // 检测状态变化（防抖处理）
    if (reading != lastButtonState)
    {
        lastDebounceTime = millis();
    }

    // 如果状态稳定超过防抖延迟
    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY)
    {
        // 如果状态确实改变了
        if (reading != buttonState)
        {
            buttonState = reading;

            // 按钮被按下（从LOW变为HIGH - NC按钮逻辑反转）
            if (buttonState == HIGH)
            {
                buttonPressed = true;
                buttonPressTime = millis();
                longPressHandled = false;
                Serial.println("[Button] ↓ Pressed");
            }
            // 按钮被释放（从HIGH变为LOW - NC按钮逻辑反转）
            else if (buttonPressed)
            {
                unsigned long pressDuration = millis() - buttonPressTime;
                buttonPressed = false;

                Serial.print("[Button] ↑ Released (");
                Serial.print(pressDuration);
                Serial.println("ms)");

                // 如果长按已经处理过，就不再处理短按
                if (!longPressHandled)
                {
                    if (pressDuration < LONG_PRESS_TIME)
                    {
                        // 短按：切换模式
                        handleShortPress();
                    }
                }
            }
        }
    }

    // 检测长按
    if (buttonPressed && !longPressHandled)
    {
        unsigned long pressDuration = millis() - buttonPressTime;
        if (pressDuration >= LONG_PRESS_TIME)
        {
            // 长按：开关灯
            Serial.println("[Button] ⏱️ Long press (2s) triggered");
            handleLongPress();
            longPressHandled = true;
        }
    }

    lastButtonState = reading;
}

void ButtonManager::handleShortPress()
{
    Serial.println("[Button] 🔄 Short press → Cycling mode");

    // 根据当前控制器选择对应的controller
    if (*currentMode == MODE_LOCAL)
    {
        // 获取当前模式
        LightMode currentLightMode = localController->getMode();
        String nextModeStr;

        // 循环切换模式: Timer → Weather → Idle → Music → Timer
        switch (currentLightMode)
        {
        case MODE_TIMER:
            nextModeStr = "weather";
            break;
        case MODE_WEATHER:
            nextModeStr = "idle";
            break;
        case MODE_IDLE:
            nextModeStr = "music";
            break;
        case MODE_MUSIC:
        default:
            nextModeStr = "timer";
            break;
        }

        // 设置新模式
        localController->setMode(nextModeStr);

        // 通过MQTT发布新模式
        mqtt->publish("mode", nextModeStr.c_str(), true);

        Serial.print("[Button]    ✓ Mode: ");
        Serial.println(nextModeStr);
    }
    else if (*currentMode == MODE_LUMINAIRE)
    {
        // 获取当前模式
        LuminaireMode currentLumiMode = luminaireController->getMode();
        String nextModeStr;

        // 循环切换模式
        switch (currentLumiMode)
        {
        case LUMI_MODE_TIMER:
            nextModeStr = "weather";
            break;
        case LUMI_MODE_WEATHER:
            nextModeStr = "idle";
            break;
        case LUMI_MODE_IDLE:
            nextModeStr = "music";
            break;
        case LUMI_MODE_MUSIC:
        default:
            nextModeStr = "timer";
            break;
        }

        // 发送MQTT消息（让handleMQTTMessage处理）
        mqtt->publish("mode", nextModeStr.c_str(), true);

        Serial.print("[Button]    ✓ Luminaire mode: ");
        Serial.println(nextModeStr);
    }
}

void ButtonManager::handleLongPress()
{
    Serial.println("[Button] 💡 Long press → Toggle light");

    // 根据当前控制器选择对应的controller
    if (*currentMode == MODE_LOCAL)
    {
        // 切换灯的状态
        bool currentlyOn = localController->isOn();

        if (currentlyOn)
        {
            localController->turnOff();
            mqtt->publish("status", "off", true);
            Serial.println("[Button]    ✓ Light OFF");
        }
        else
        {
            localController->turnOn();
            mqtt->publish("status", "on", true);
            Serial.println("[Button]    ✓ Light ON");
        }
    }
    else if (*currentMode == MODE_LUMINAIRE)
    {
        // 切换Luminaire的状态
        bool currentlyOn = luminaireController->isOn();

        if (currentlyOn)
        {
            mqtt->publish("status", "off", true);
            Serial.println("[Button]    ✓ Luminaire OFF");
        }
        else
        {
            mqtt->publish("status", "on", true);
            Serial.println("[Button]    ✓ Luminaire ON");
        }
    }
}
