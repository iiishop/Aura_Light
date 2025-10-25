

import mqttManager from './mqtt.js';
import ui from './ui.js';
import { MQTT_CONFIG } from './config.js';
import WeatherManager from './weather.js';

class AuraLightDashboard {
    constructor() {
        this.weatherManager = new WeatherManager();
        this.init();
    }


    init() {
        console.log('[App] Initializing Aura Light Dashboard...');


        ui.init();

        // 初始化音频监测
        ui.initAudioMonitor();


        this.setupMQTTCallbacks();


        this.setupUIEvents();

        console.log('[App] Dashboard ready!');
    }


    setupMQTTCallbacks() {

        mqttManager.on('connect', () => {
            console.log('[App] MQTT connect callback triggered');
            ui.updateConnectionStatus(true);
            ui.addLog('success', 'System', '✓ Connected to MQTT broker');
        });


        mqttManager.on('disconnect', () => {
            console.log('[App] MQTT disconnect callback triggered');
            ui.updateConnectionStatus(false);
            ui.addLog('error', 'System', '✗ Disconnected from MQTT broker');
        });


        mqttManager.on('message', (topic, message) => {
            console.log('========================================');
            console.log('[App] RAW MESSAGE RECEIVED');
            console.log('[App] Topic:', topic);
            console.log('[App] Message:', message);
            console.log('========================================');
            this.handleMessage(topic, message);
            ui.addLog('received', topic, message);
        });


        mqttManager.on('error', (error) => {
            console.error('[App] MQTT Error:', error);
            ui.addLog('error', 'System', `Error: ${error.message}`);
        });
    }


    handleMessage(topic, message) {
        console.log('[App] Handling message:', topic, '=', message);


        const parts = topic.split('/');
        const lastPart = parts[parts.length - 1];
        const secondLastPart = parts[parts.length - 2];

        console.log('[App] Topic parts:', parts);
        console.log('[App] Last part:', lastPart);
        console.log('[App] Second last part:', secondLastPart);


        if (topic.endsWith('/status')) {
            console.log('[App] → STATUS message');
            ui.updateLightStatus(message);
        }


        else if (topic.endsWith('/mode')) {
            console.log('[App] → MODE message');
            ui.updateMode(message);
        }


        else if (topic.endsWith('/controller')) {
            console.log('[App] → CONTROLLER message');
            ui.updateController(message);
        }


        else if (topic.includes('/debug/')) {
            console.log('[App] → DEBUG message');

            ui.updateDebugStatus(true);


            if (topic.endsWith('/debug/color')) {

                const match = message.match(/^(\d+):#([0-9A-Fa-f]{6})$/);
                if (match) {
                    const pixelIndex = parseInt(match[1]);
                    const color = '#' + match[2].toUpperCase();
                    console.log('[App] DEBUG color update - pixel:', pixelIndex, 'color:', color);
                    ui.updatePixelColor(pixelIndex, color);
                }
            } else if (topic.endsWith('/debug/brightness')) {

                const match = message.match(/^(\d+):(\d+)$/);
                if (match) {
                    const pixelIndex = parseInt(match[1]);
                    const brightness = parseInt(match[2]);
                    console.log('[App] DEBUG brightness update - pixel:', pixelIndex, 'brightness:', brightness);
                    ui.updatePixelBrightness(pixelIndex, brightness);
                }
            } else if (topic.endsWith('/debug/index')) {
                if (message.toLowerCase() === 'clear') {
                    console.log('[App] DEBUG cleared');
                    ui.updateDebugStatus(false);
                    ui.updateVisualization();
                }
            }
        }

        // MAX9814 音频数据（优先检查，因为路径包含 /info/audio/）
        else if (topic.includes('/info/audio/')) {
            console.log('[App] → AUDIO message (via /info/audio/ path)');
            this.handleAudioMessage(topic, message);
        }


        else if (topic.includes('/info/')) {
            console.log('[App] → INFO message');


            if (topic.endsWith('/info/weather')) {
                console.log('[App] → WEATHER INFO message');
                this.weatherManager.handleWeatherData(message);
            } else {
                this.handleInfoMessage(topic, secondLastPart, lastPart, message);
            }
        }
    }


    // 处理音频数据
    handleAudioMessage(topic, message) {
        console.log('[App] handleAudioMessage called with:', topic, message);
        try {
            if (topic.endsWith('/info/audio/data') || topic.endsWith('/audio/data')) {
                console.log('[App] Processing audio/data...');
                // 期望格式: "raw,volume,vuLevel,band0,band1,...,band11"
                // 例如: "512,65.2,3,0.4,0.5,0.6,0.7,0.6,0.5,0.4,0.3,0.2,0.1,0.05,0.03"
                const parts = message.split(',');
                console.log('[App] Message parts:', parts);

                const audioData = {
                    raw: parseInt(parts[0]),
                    volume: parseFloat(parts[1]),
                    vuLevel: parseInt(parts[2]),
                    spectrum: []
                };

                // 提取 12 个频段数据
                for (let i = 3; i < 15 && i < parts.length; i++) {
                    audioData.spectrum.push(parseFloat(parts[i]));
                }

                console.log('[App] Parsed audio data:', audioData);
                ui.updateAudioMonitor(audioData);
            } else if (topic.endsWith('/info/audio/volume_range') || topic.endsWith('/audio/volume_range')) {
                // 音量范围更新: "30,120"
                const parts = message.split(',');
                if (parts.length === 2) {
                    ui.updateAudioMonitor({
                        minDb: parseFloat(parts[0]),
                        maxDb: parseFloat(parts[1])
                    });
                }
            }
        } catch (error) {
            console.error('[App] Error parsing audio data:', error);
        }
    }


    handleInfoMessage(fullTopic, category, field, value) {
        console.log('[App] INFO - category:', category, 'field:', field, 'value:', value);


        if (category === 'info' && field === 'controller') {
            console.log('[App] Controller update from Arduino:', value);
            ui.updateController(value);
            return;
        }



        const categoryCapital = category.charAt(0).toUpperCase() + category.slice(1).toLowerCase();
        let fieldCapital = field.charAt(0).toUpperCase() + field.slice(1).toLowerCase();


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


        if (category === 'location' && field === 'city') {
            console.log('[App] City detected:', value);
            this.weatherManager.setCity(value);
        }
    }


    setupUIEvents() {

        ui.elements.connectBtn.addEventListener('click', () => {
            console.log('[App] Connect button clicked');
            this.connect();
        });


        ui.elements.disconnectBtn.addEventListener('click', () => {
            console.log('[App] Disconnect button clicked');
            this.disconnect();
        });


        ui.elements.refreshInfoBtn.addEventListener('click', () => {
            console.log('[App] Refresh INFO button clicked');
            this.requestInfo();
        });


        ui.elements.turnOnBtn.addEventListener('click', () => {
            this.publishStatus('on');
        });


        ui.elements.turnOffBtn.addEventListener('click', () => {
            this.publishStatus('off');
        });


        ui.elements.modeButtons.forEach(btn => {
            btn.addEventListener('click', () => {
                const mode = btn.dataset.mode;
                this.publishMode(mode);
            });
        });


        ui.onControllerSwitch = (controller) => {
            console.log('[App] Controller switch requested:', controller);
            this.publishController(controller);
        };


        ui.elements.applyDebugBtn.addEventListener('click', () => {
            this.applyDebug();
        });


        ui.elements.clearDebugBtn.addEventListener('click', () => {
            this.clearDebug();
        });


        ui.elements.usernameInput.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') {
                this.connect();
            }
        });
    }


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


    disconnect() {
        mqttManager.disconnect();
        ui.addLog('info', 'System', 'Disconnecting...');
    }


    requestInfo() {
        console.log('[App] Requesting INFO from device...');

        if (mqttManager.publish('/refresh', 'info')) {
            ui.addLog('sent', 'refresh', 'info');
            console.log('[App] INFO refresh request sent');
        } else {
            console.error('[App] Failed to send INFO request');
        }
    }


    publishStatus(status) {
        if (mqttManager.publish(MQTT_CONFIG.topics.status, status)) {
            ui.addLog('sent', 'status', status);

            ui.updateLightStatus(status);
        }
    }


    publishMode(mode) {
        if (mqttManager.publish(MQTT_CONFIG.topics.mode, mode)) {
            ui.addLog('sent', 'mode', mode);

            ui.updateMode(mode);
        }
    }


    publishController(controller) {
        if (mqttManager.publish(MQTT_CONFIG.topics.controller, controller)) {
            ui.addLog('sent', 'controller', controller);

            ui.updateController(controller);
        }
    }


    applyDebug() {
        const settings = ui.getDebugSettings();


        const colorMsg = `${settings.index}:${settings.color}`;
        mqttManager.publish(MQTT_CONFIG.topics.debugColor, colorMsg);
        ui.addLog('sent', 'debug/color', colorMsg);


        const brightnessMsg = `${settings.index}:${settings.brightness}`;
        mqttManager.publish(MQTT_CONFIG.topics.debugBrightness, brightnessMsg);
        ui.addLog('sent', 'debug/brightness', brightnessMsg);

        ui.updateDebugStatus(true);
    }


    clearDebug() {
        if (mqttManager.publish(MQTT_CONFIG.topics.debugIndex, 'clear')) {
            ui.addLog('sent', 'debug/index', 'clear');
            ui.updateDebugStatus(false);
        }
    }
}

document.addEventListener('DOMContentLoaded', () => {
    new AuraLightDashboard();
});
