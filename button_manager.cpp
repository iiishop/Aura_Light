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
      longPressHandled(false),
      lastClickTime(0),
      clickCount(0),
      statusLED(nullptr)
{
}

ButtonManager::~ButtonManager()
{
    if (statusLED != nullptr)
    {
        statusLED->clear();
        statusLED->show();
        delete statusLED;
        statusLED = nullptr;
    }
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
    Serial.println("[Button] - Double click: Switch controller (Local ↔ Luminaire)");

    // 初始化 NeoPixel 状态指示器
    statusLED = new Adafruit_NeoPixel(1, STATUS_LED_PIN, NEO_GRB + NEO_KHZ800);
    statusLED->begin();
    statusLED->clear();
    statusLED->show();
    Serial.println("[Button] ✓ NeoPixel status indicator initialized on pin 0");
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
                        // 检测双击
                        unsigned long timeSinceLastClick = millis() - lastClickTime;

                        if (timeSinceLastClick < DOUBLE_CLICK_TIME && clickCount == 1)
                        {
                            // 双击检测到
                            Serial.print("[Button] ⚡⚡ Double click detected (");
                            Serial.print(timeSinceLastClick);
                            Serial.println("ms interval)");
                            clickCount = 0;
                            lastClickTime = 0;
                            handleDoubleClick();
                        }
                        else
                        {
                            // 第一次点击或超时
                            if (clickCount == 0)
                            {
                                Serial.println("[Button] 🖱️ Click #1, waiting for second click...");
                            }
                            clickCount = 1;
                            lastClickTime = millis();
                        }
                    }
                }
            }
        }
    }

    // 检测单击超时（如果只有一次点击且超过双击时间窗口）
    if (clickCount == 1 && (millis() - lastClickTime) > DOUBLE_CLICK_TIME)
    {
        Serial.println("[Button] ⏱️ Double click timeout, executing single click");
        clickCount = 0;
        handleShortPress();
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
        mqtt->publishMode(nextModeStr.c_str());

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

        // 发送MQTT消息（LuminaireController通过handleMQTTMessage处理）
        mqtt->publishMode(nextModeStr.c_str());

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
            mqtt->publishStatus("off");
            Serial.println("[Button]    ✓ Light OFF");
        }
        else
        {
            localController->turnOn();
            mqtt->publishStatus("on");
            Serial.println("[Button]    ✓ Light ON");
        }
    }
    else if (*currentMode == MODE_LUMINAIRE)
    {
        // 切换Luminaire的状态
        bool currentlyOn = luminaireController->isOn();

        if (currentlyOn)
        {
            mqtt->publishStatus("off");
            Serial.println("[Button]    ✓ Luminaire OFF");
        }
        else
        {
            mqtt->publishStatus("on");
            Serial.println("[Button]    ✓ Luminaire ON");
        }
    }
}

void ButtonManager::handleDoubleClick()
{
    Serial.println("[Button] ⚡⚡ Double click → Switch controller");

    if (*currentMode == MODE_LOCAL)
    {
        // 切换到 Luminaire
        *currentMode = MODE_LUMINAIRE;
        mqtt->publishInfo("controller", "luminaire", true);

        // 闪红光
        flashNeoPixel(statusLED->Color(255, 0, 0), 300);

        // 更新状态指示器
        updateLuminaireStatus();

        Serial.println("[Button]    ✓ Switched to LUMINAIRE");
    }
    else
    {
        // 切换到 Local
        *currentMode = MODE_LOCAL;
        mqtt->publishInfo("controller", "local", true);

        // 闪绿光后熄灭
        flashNeoPixel(statusLED->Color(0, 255, 0), 300);
        statusLED->clear();
        statusLED->show();

        Serial.println("[Button]    ✓ Switched to LOCAL");
    }
}

void ButtonManager::flashNeoPixel(uint32_t color, int duration)
{
    statusLED->setPixelColor(0, color);
    statusLED->show();
    delay(duration);
    statusLED->clear();
    statusLED->show();
}

void ButtonManager::updateLuminaireStatus()
{
    // 仅在 Luminaire 模式下更新状态指示
    if (*currentMode != MODE_LUMINAIRE)
    {
        statusLED->clear();
        statusLED->show();
        return;
    }

    // 检查 Luminaire 是否开启
    if (!luminaireController->isOn())
    {
        statusLED->clear();
        statusLED->show();
        return;
    }

    // 根据模式显示不同颜色
    LuminaireMode mode = luminaireController->getMode();
    uint32_t color = 0;

    switch (mode)
    {
    case LUMI_MODE_IDLE:
        color = statusLED->Color(0, 0, 255); // 蓝色
        break;
    case LUMI_MODE_WEATHER:
        color = statusLED->Color(0, 255, 0); // 绿色
        break;
    case LUMI_MODE_TIMER:
        color = statusLED->Color(255, 0, 0); // 红色
        break;
    case LUMI_MODE_MUSIC:
        color = statusLED->Color(255, 255, 0); // 黄色
        break;
    default:
        color = statusLED->Color(0, 0, 255); // 默认蓝色
        break;
    }

    statusLED->setPixelColor(0, color);
    statusLED->show();
}

void ButtonManager::updateStatusLED()
{
    updateLuminaireStatus();
}
