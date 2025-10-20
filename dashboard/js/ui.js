/**
 * UI管理器模块
 * 处理所有UI更新和用户交互
 */

class UIManager {
    constructor() {
        this.elements = {};
        this.state = {
            connected: false,
            lightOn: false,
            currentMode: 'idle',
            currentController: 'local',
            pixelCount: 8,
            debugActive: false
        };
    }

    /**
     * 初始化UI元素引用
     */
    init() {
        // 连接相关
        this.elements.connectionStatus = document.getElementById('connectionStatus');
        this.elements.statusIndicator = document.getElementById('statusIndicator');
        this.elements.statusText = document.getElementById('statusText');
        this.elements.usernameInput = document.getElementById('username');
        this.elements.connectBtn = document.getElementById('connectBtn');
        this.elements.disconnectBtn = document.getElementById('disconnectBtn');

        // 灯光状态
        this.elements.lightStatus = document.getElementById('lightStatus');
        this.elements.turnOnBtn = document.getElementById('turnOnBtn');
        this.elements.turnOffBtn = document.getElementById('turnOffBtn');

        // 模式控制
        this.elements.currentMode = document.getElementById('currentMode');
        this.elements.modeButtons = document.querySelectorAll('.mode-btn');

        // 控制器切换
        this.elements.currentController = document.getElementById('currentController');
        this.elements.controllerButtons = document.querySelectorAll('.controller-btn');

        // INFO显示
        this.elements.refreshInfoBtn = document.getElementById('refreshInfoBtn');
        this.elements.infoWifiSSID = document.getElementById('infoWifiSSID');
        this.elements.infoWifiIP = document.getElementById('infoWifiIP');
        this.elements.infoWifiRSSI = document.getElementById('infoWifiRSSI');
        this.elements.infoWifiMAC = document.getElementById('infoWifiMAC');
        this.elements.infoLighterNumber = document.getElementById('infoLighterNumber');
        this.elements.infoLighterPin = document.getElementById('infoLighterPin');
        this.elements.infoSystemVersion = document.getElementById('infoSystemVersion');
        this.elements.infoSystemUptime = document.getElementById('infoSystemUptime');
        this.elements.infoLocationCity = document.getElementById('infoLocationCity');

        // 灯光可视化
        this.elements.lightVisualization = document.getElementById('lightVisualization');

        // DEBUG控制
        this.elements.debugPixelIndex = document.getElementById('debugPixelIndex');
        this.elements.debugColor = document.getElementById('debugColor');
        this.elements.debugColorHex = document.getElementById('debugColorHex');
        this.elements.debugBrightness = document.getElementById('debugBrightness');
        this.elements.brightnessValue = document.getElementById('brightnessValue');
        this.elements.applyDebugBtn = document.getElementById('applyDebugBtn');
        this.elements.clearDebugBtn = document.getElementById('clearDebugBtn');
        this.elements.debugStatus = document.getElementById('debugStatus');

        // MQTT日志
        this.elements.mqttLog = document.getElementById('mqttLog');
        this.elements.clearLogBtn = document.getElementById('clearLogBtn');
        this.elements.autoScrollLog = document.getElementById('autoScrollLog');

        // 硬编码username为ucfninn
        this.elements.usernameInput.value = 'ucfninn';
        console.log('[UI] Username set to: ucfninn');

        this.setupEventListeners();
    }

    /**
     * 设置事件监听器
     */
    setupEventListeners() {
        // 颜色选择器同步
        this.elements.debugColor.addEventListener('input', (e) => {
            this.elements.debugColorHex.value = e.target.value.toUpperCase();
        });

        this.elements.debugColorHex.addEventListener('input', (e) => {
            const hex = e.target.value;
            if (/^#[0-9A-F]{6}$/i.test(hex)) {
                this.elements.debugColor.value = hex;
            }
        });

        // 亮度滑块
        this.elements.debugBrightness.addEventListener('input', (e) => {
            this.elements.brightnessValue.textContent = e.target.value;
        });

        // 清除日志
        this.elements.clearLogBtn.addEventListener('click', () => {
            this.clearLog();
        });

        // 控制器切换按钮
        this.elements.controllerButtons.forEach(btn => {
            btn.addEventListener('click', (e) => {
                if (!btn.disabled) {
                    const controller = btn.dataset.controller;
                    // 这个回调会在 app.js 中设置
                    if (this.onControllerSwitch) {
                        this.onControllerSwitch(controller);
                    }
                }
            });
        });
    }

    /**
     * 更新连接状态
     */
    updateConnectionStatus(connected) {
        this.state.connected = connected;

        if (connected) {
            this.elements.statusIndicator.classList.add('connected');
            this.elements.statusText.textContent = 'Connected';
            this.elements.connectBtn.disabled = true;
            this.elements.disconnectBtn.disabled = false;
            this.elements.usernameInput.disabled = true;

            // 启用控制按钮
            this.elements.refreshInfoBtn.disabled = false;
            this.elements.turnOnBtn.disabled = false;
            this.elements.turnOffBtn.disabled = false;
            this.elements.modeButtons.forEach(btn => btn.disabled = false);
            this.elements.controllerButtons.forEach(btn => btn.disabled = false);
            this.elements.debugPixelIndex.disabled = false;
            this.elements.debugColor.disabled = false;
            this.elements.debugColorHex.disabled = false;
            this.elements.debugBrightness.disabled = false;
            this.elements.applyDebugBtn.disabled = false;
            this.elements.clearDebugBtn.disabled = false;

        } else {
            this.elements.statusIndicator.classList.remove('connected');
            this.elements.statusText.textContent = 'Disconnected';
            this.elements.connectBtn.disabled = false;
            this.elements.disconnectBtn.disabled = true;
            this.elements.usernameInput.disabled = false;

            // 禁用控制按钮
            this.elements.refreshInfoBtn.disabled = true;
            this.elements.turnOnBtn.disabled = true;
            this.elements.turnOffBtn.disabled = true;
            this.elements.modeButtons.forEach(btn => btn.disabled = true);
            this.elements.controllerButtons.forEach(btn => btn.disabled = true);
            this.elements.debugPixelIndex.disabled = true;
            this.elements.debugColor.disabled = true;
            this.elements.debugColorHex.disabled = true;
            this.elements.debugBrightness.disabled = true;
            this.elements.applyDebugBtn.disabled = true;
            this.elements.clearDebugBtn.disabled = true;
        }
    }

    /**
     * 更新灯光状态
     */
    updateLightStatus(status) {
        console.log('[UI] updateLightStatus called with:', status);
        console.log('[UI] Status type:', typeof status);

        this.state.lightOn = (status === 'on');
        console.log('[UI] Light state set to:', this.state.lightOn);

        if (this.state.lightOn) {
            this.elements.lightStatus.textContent = 'ON';
            this.elements.lightStatus.classList.add('on');
            console.log('[UI] ✓ Light status updated to ON');
        } else {
            this.elements.lightStatus.textContent = 'OFF';
            this.elements.lightStatus.classList.remove('on');
            console.log('[UI] ✓ Light status updated to OFF');
        }

        this.updateVisualization();
    }

    /**
     * 更新模式显示
     */
    updateMode(mode) {
        console.log('[UI] updateMode called with:', mode);
        console.log('[UI] Mode type:', typeof mode);

        this.state.currentMode = mode.toLowerCase();
        console.log('[UI] Current mode set to:', this.state.currentMode);

        // 更新徽章
        this.elements.currentMode.textContent = mode.toUpperCase();
        this.elements.currentMode.className = 'mode-badge ' + this.state.currentMode;
        console.log('[UI] ✓ Mode badge updated to:', mode.toUpperCase());

        // 更新按钮状态
        this.elements.modeButtons.forEach(btn => {
            if (btn.dataset.mode === this.state.currentMode) {
                btn.classList.add('active');
                console.log('[UI] ✓ Mode button activated:', btn.dataset.mode);
            } else {
                btn.classList.remove('active');
            }
        });

        this.updateVisualization();
    }

    /**
     * 更新控制器显示
     */
    updateController(controller) {
        console.log('[UI] updateController called with:', controller);

        this.state.currentController = controller.toLowerCase();

        // 更新徽章
        this.elements.currentController.textContent = controller.toUpperCase();

        // 更新按钮状态
        this.elements.controllerButtons.forEach(btn => {
            if (btn.dataset.controller === this.state.currentController) {
                btn.classList.add('active');
            } else {
                btn.classList.remove('active');
            }
        });

        // 更新像素数量
        if (this.state.currentController === 'luminaire') {
            this.state.pixelCount = 72; // Luminaire has 72 LEDs
        } else {
            // 从INFO中读取本地灯数量，或使用默认值
            const localCount = this.elements.infoLighterNumber?.textContent;
            this.state.pixelCount = parseInt(localCount) || 8;
        }

        this.updateVisualization();
        console.log('[UI] ✓ Controller updated to:', controller);
    }

    /**
     * 更新INFO信息
     * @param {string} field - 字段名，已包含info前缀，如 "infoWifiSSID"
     * @param {string} value - 字段值
     */
    updateInfo(field, value) {
        console.log('[UI] updateInfo called - field:', field, 'value:', value);

        // field 已经包含 'info' 前缀了，直接使用
        const elementKey = field;
        console.log('[UI] Looking for element:', elementKey);

        const element = this.elements[elementKey];

        if (element) {
            console.log('[UI] ✓ Element found, updating to:', value);
            element.textContent = value;

            // 特殊处理 - 注意这里也要改
            if (field === 'infoLighterNumber') {
                this.state.pixelCount = parseInt(value) || 1;
                this.updatePixelSelector();
                this.updateVisualization();
            }
        } else {
            console.warn('[UI] ✗ Element NOT found for:', elementKey);
            console.log('[UI] Available info elements:', Object.keys(this.elements).filter(k => k.startsWith('info')));
        }
    }

    /**
     * 更新像素选择器
     */
    updatePixelSelector() {
        this.elements.debugPixelIndex.innerHTML = '';
        for (let i = 0; i < this.state.pixelCount; i++) {
            const option = document.createElement('option');
            option.value = i;
            option.textContent = `Pixel ${i}`;
            this.elements.debugPixelIndex.appendChild(option);
        }
    }

    /**
     * 更新灯光可视化
     */
    updateVisualization() {
        this.elements.lightVisualization.innerHTML = '';

        // 设置像素数量属性，用于响应式布局
        this.elements.lightVisualization.setAttribute('data-pixel-count', this.state.pixelCount);

        // 如果是Luminaire控制器，显示外部网页
        if (this.state.currentController === 'luminaire') {
            this.elements.lightVisualization.classList.add('luminaire-iframe');
            this.elements.lightVisualization.classList.remove('luminaire-grid');

            const iframe = document.createElement('iframe');
            iframe.src = 'https://www.iot.io/projects/lumi/';
            iframe.style.width = '100%';
            iframe.style.height = '100%';
            iframe.style.border = 'none';
            iframe.style.borderRadius = '8px';
            iframe.setAttribute('allowfullscreen', '');

            this.elements.lightVisualization.appendChild(iframe);
            return; // 不再创建像素格子
        } else {
            this.elements.lightVisualization.classList.remove('luminaire-grid');
            this.elements.lightVisualization.classList.remove('luminaire-iframe');
        }

        for (let i = 0; i < this.state.pixelCount; i++) {
            const pixel = document.createElement('div');
            pixel.className = 'pixel';
            pixel.dataset.index = i;
            pixel.setAttribute('data-index', i);

            if (this.state.lightOn) {
                pixel.classList.add('on');

                // 根据模式设置颜色
                const color = this.getModeColor(this.state.currentMode);
                pixel.style.backgroundColor = color;
                pixel.style.color = color; // 用于currentColor
                pixel.style.boxShadow = `0 0 15px ${color}, 0 0 30px ${color}`;

                // IDLE模式添加呼吸效果
                if (this.state.currentMode === 'idle') {
                    pixel.classList.add('breathing');
                }
            } else {
                pixel.style.backgroundColor = '#2a2a2a';
                pixel.style.color = '#2a2a2a';
                pixel.style.boxShadow = 'inset 0 3px 6px rgba(0,0,0,0.6)';
            }

            this.elements.lightVisualization.appendChild(pixel);
        }
    }

    /**
     * 更新单个像素的颜色
     */
    updatePixelColor(index, color) {
        const pixel = this.elements.lightVisualization.querySelector(`[data-index="${index}"]`);
        if (pixel && this.state.lightOn) {
            pixel.style.backgroundColor = color;
            pixel.style.color = color; // 用于currentColor
            pixel.style.boxShadow = `0 0 20px ${color}, 0 0 40px ${color}, 0 0 60px ${color}`;
            pixel.classList.remove('breathing'); // 移除呼吸效果
            pixel.classList.add('debug-mode'); // 添加DEBUG模式标记
            console.log('[UI] ✓ Pixel', index, 'color updated to', color);
        }
    }

    /**
     * 更新单个像素的亮度
     */
    updatePixelBrightness(index, brightness) {
        const pixel = this.elements.lightVisualization.querySelector(`[data-index="${index}"]`);
        if (pixel && this.state.lightOn) {
            const opacity = Math.max(0.2, brightness / 255); // 最小20%确保可见
            pixel.style.opacity = opacity;
            pixel.classList.add('debug-mode'); // 添加DEBUG模式标记
            console.log('[UI] ✓ Pixel', index, 'brightness updated to', brightness, '(opacity:', opacity, ')');
        }
    }

    /**
     * 获取模式对应的颜色
     */
    getModeColor(mode) {
        const colors = {
            'timer': '#ff4444',
            'weather': '#44ff44',
            'idle': '#4444ff'
        };
        return colors[mode] || '#ffffff';
    }

    /**
     * 更新DEBUG状态
     */
    updateDebugStatus(active) {
        this.state.debugActive = active;

        if (active) {
            this.elements.debugStatus.textContent = 'DEBUG Active';
            this.elements.debugStatus.classList.add('active');
        } else {
            this.elements.debugStatus.textContent = 'DEBUG Inactive';
            this.elements.debugStatus.classList.remove('active');
        }
    }

    /**
     * 添加MQTT日志
     */
    addLog(type, topic, message) {
        const timestamp = new Date().toLocaleTimeString();
        const entry = document.createElement('div');
        entry.className = 'log-entry';

        entry.innerHTML = `
            <span class="log-timestamp">${timestamp}</span>
            <span class="log-type log-type-${type}">[${type.toUpperCase()}]</span>
            <span class="log-topic">${topic}</span>: 
            <span class="log-message">${message}</span>
        `;

        this.elements.mqttLog.appendChild(entry);

        // 自动滚动
        if (this.elements.autoScrollLog.checked) {
            this.elements.mqttLog.scrollTop = this.elements.mqttLog.scrollHeight;
        }

        // 限制日志数量
        const maxLogs = 100;
        while (this.elements.mqttLog.children.length > maxLogs) {
            this.elements.mqttLog.removeChild(this.elements.mqttLog.firstChild);
        }
    }

    /**
     * 清除日志
     */
    clearLog() {
        this.elements.mqttLog.innerHTML = '';
    }

    /**
     * 获取用户输入
     */
    getUsername() {
        return this.elements.usernameInput.value.trim();
    }

    /**
     * 获取DEBUG设置
     */
    getDebugSettings() {
        return {
            index: this.elements.debugPixelIndex.value,
            color: this.elements.debugColor.value,
            brightness: this.elements.debugBrightness.value
        };
    }

    /**
     * 保存username到localStorage
     */
    saveUsername(username) {
        localStorage.setItem('auralight_username', username);
    }
}

export default new UIManager();
