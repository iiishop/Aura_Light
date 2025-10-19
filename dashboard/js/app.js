/**
 * 主应用模块
 * 协调所有模块工作
 */

import mqttManager from './mqtt.js';
import ui from './ui.js';
import { MQTT_CONFIG } from './config.js';

class AuraLightDashboard {
    constructor() {
        this.init();
    }

    /**
     * 初始化应用
     */
    init() {
        console.log('[App] Initializing Aura Light Dashboard...');

        // 初始化UI
        ui.init();

        // 设置MQTT回调
        this.setupMQTTCallbacks();

        // 设置UI事件
        this.setupUIEvents();

        console.log('[App] Dashboard ready!');
    }

    /**
     * 设置MQTT回调
     */
    setupMQTTCallbacks() {
        // 连接成功
        mqttManager.on('connect', () => {
            console.log('[App] MQTT connect callback triggered');
            ui.updateConnectionStatus(true);
            ui.addLog('success', 'System', '✓ Connected to MQTT broker');
        });

        // 断开连接
        mqttManager.on('disconnect', () => {
            console.log('[App] MQTT disconnect callback triggered');
            ui.updateConnectionStatus(false);
            ui.addLog('error', 'System', '✗ Disconnected from MQTT broker');
        });

        // 接收消息
        mqttManager.on('message', (topic, message) => {
            console.log('========================================');
            console.log('[App] RAW MESSAGE RECEIVED');
            console.log('[App] Topic:', topic);
            console.log('[App] Message:', message);
            console.log('========================================');
            this.handleMessage(topic, message);
            ui.addLog('received', topic, message);
        });

        // 错误处理
        mqttManager.on('error', (error) => {
            console.error('[App] MQTT Error:', error);
            ui.addLog('error', 'System', `Error: ${error.message}`);
        });
    }

    /**
     * 处理接收到的MQTT消息
     */
    handleMessage(topic, message) {
        console.log('[App] Handling message:', topic, '=', message);

        // 解析topic
        const parts = topic.split('/');
        const lastPart = parts[parts.length - 1];
        const secondLastPart = parts[parts.length - 2];

        console.log('[App] Topic parts:', parts);
        console.log('[App] Last part:', lastPart);
        console.log('[App] Second last part:', secondLastPart);

        // STATUS消息
        if (topic.endsWith('/status')) {
            console.log('[App] → STATUS message');
            ui.updateLightStatus(message);
        }

        // MODE消息
        else if (topic.endsWith('/mode')) {
            console.log('[App] → MODE message');
            ui.updateMode(message);
        }

        // DEBUG消息
        else if (topic.includes('/debug/')) {
            console.log('[App] → DEBUG message');
            // DEBUG消息表示DEBUG模式激活
            ui.updateDebugStatus(true);
        }

        // INFO消息
        else if (topic.includes('/info/')) {
            console.log('[App] → INFO message');
            this.handleInfoMessage(topic, secondLastPart, lastPart, message);
        }
    }

    /**
     * 处理INFO消息
     * topic格式: student/CASA0014/ucfninn/info/wifi/ssid
     */
    handleInfoMessage(fullTopic, category, field, value) {
        console.log('[App] INFO - category:', category, 'field:', field, 'value:', value);

        // 构建字段名: info + Category + Field (驼峰命名)
        // wifi/ssid → infoWifiSsid → infoWifiSSID (特殊处理)
        const categoryCapital = category.charAt(0).toUpperCase() + category.slice(1).toLowerCase();
        let fieldCapital = field.charAt(0).toUpperCase() + field.slice(1).toLowerCase();

        // 特殊字段名处理
        const fieldMap = {
            'ssid': 'SSID',
            'ip': 'IP',
            'rssi': 'RSSI',
            'mac': 'MAC'
        };

        if (fieldMap[field.toLowerCase()]) {
            fieldCapital = fieldMap[field.toLowerCase()];
        }

        const fieldName = 'info' + categoryCapital + fieldCapital;
        console.log('[App] Mapped field name:', fieldName);

        ui.updateInfo(fieldName, value);
    }

    /**
     * 设置UI事件
     */
    setupUIEvents() {
        // 连接按钮
        ui.elements.connectBtn.addEventListener('click', () => {
            console.log('[App] Connect button clicked');
            this.connect();
        });

        // 断开按钮
        ui.elements.disconnectBtn.addEventListener('click', () => {
            console.log('[App] Disconnect button clicked');
            this.disconnect();
        });

        // 刷新INFO按钮
        ui.elements.refreshInfoBtn.addEventListener('click', () => {
            console.log('[App] Refresh INFO button clicked');
            this.requestInfo();
        });

        // 开灯按钮
        ui.elements.turnOnBtn.addEventListener('click', () => {
            this.publishStatus('on');
        });

        // 关灯按钮
        ui.elements.turnOffBtn.addEventListener('click', () => {
            this.publishStatus('off');
        });

        // 模式按钮
        ui.elements.modeButtons.forEach(btn => {
            btn.addEventListener('click', () => {
                const mode = btn.dataset.mode;
                this.publishMode(mode);
            });
        });

        // 应用DEBUG按钮
        ui.elements.applyDebugBtn.addEventListener('click', () => {
            this.applyDebug();
        });

        // 清除DEBUG按钮
        ui.elements.clearDebugBtn.addEventListener('click', () => {
            this.clearDebug();
        });

        // 回车连接
        ui.elements.usernameInput.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') {
                this.connect();
            }
        });
    }

    /**
     * 连接到MQTT
     */
    async connect() {
        const username = ui.getUsername();

        console.log('[App] Starting connection...');
        console.log('[App] Username:', username);

        if (!username) {
            console.error('[App] No username provided!');
            alert('Please enter your username (学号)');
            return;
        }

        ui.addLog('info', 'System', `Connecting as ${username}...`);
        console.log('[App] Calling mqttManager.connect()...');

        try {
            console.log('[App] Awaiting connection...');
            await mqttManager.connect(username);
            console.log('[App] ✓✓✓ Connection promise resolved ✓✓✓');
            ui.saveUsername(username);
        } catch (error) {
            console.error('[App] !!! Connection promise rejected !!!');
            console.error('[App] Error:', error);
            console.error('[App] Error message:', error.message);
            console.error('[App] Error stack:', error.stack);
            ui.addLog('error', 'System', `Connection failed: ${error.message}`);
            alert(`Connection failed: ${error.message}`);
        }
    }

    /**
     * 断开连接
     */
    disconnect() {
        mqttManager.disconnect();
        ui.addLog('info', 'System', 'Disconnecting...');
    }

    /**
     * 请求设备重新发送INFO数据
     */
    requestInfo() {
        console.log('[App] Requesting INFO from device...');
        // 发送一个refresh命令到设备
        if (mqttManager.publish('/refresh', 'info')) {
            ui.addLog('sent', 'refresh', 'info');
            console.log('[App] INFO refresh request sent');
        } else {
            console.error('[App] Failed to send INFO request');
        }
    }

    /**
     * 发布STATUS消息
     */
    publishStatus(status) {
        if (mqttManager.publish(MQTT_CONFIG.topics.status, status)) {
            ui.addLog('sent', 'status', status);
        }
    }

    /**
     * 发布MODE消息
     */
    publishMode(mode) {
        if (mqttManager.publish(MQTT_CONFIG.topics.mode, mode)) {
            ui.addLog('sent', 'mode', mode);
        }
    }

    /**
     * 应用DEBUG设置
     */
    applyDebug() {
        const settings = ui.getDebugSettings();

        // 发布颜色
        const colorMsg = `${settings.index}:${settings.color}`;
        mqttManager.publish(MQTT_CONFIG.topics.debugColor, colorMsg);
        ui.addLog('sent', 'debug/color', colorMsg);

        // 发布亮度
        const brightnessMsg = `${settings.index}:${settings.brightness}`;
        mqttManager.publish(MQTT_CONFIG.topics.debugBrightness, brightnessMsg);
        ui.addLog('sent', 'debug/brightness', brightnessMsg);

        ui.updateDebugStatus(true);
    }

    /**
     * 清除DEBUG模式
     */
    clearDebug() {
        if (mqttManager.publish(MQTT_CONFIG.topics.debugIndex, 'clear')) {
            ui.addLog('sent', 'debug/index', 'clear');
            ui.updateDebugStatus(false);
        }
    }
}

// 启动应用
document.addEventListener('DOMContentLoaded', () => {
    new AuraLightDashboard();
});
