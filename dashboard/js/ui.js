

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


    init() {

        this.elements.connectionStatus = document.getElementById('connectionStatus');
        this.elements.statusIndicator = document.getElementById('statusIndicator');
        this.elements.statusText = document.getElementById('statusText');
        this.elements.usernameInput = document.getElementById('username');
        this.elements.connectBtn = document.getElementById('connectBtn');
        this.elements.disconnectBtn = document.getElementById('disconnectBtn');


        this.elements.lightStatus = document.getElementById('lightStatus');
        this.elements.turnOnBtn = document.getElementById('turnOnBtn');
        this.elements.turnOffBtn = document.getElementById('turnOffBtn');


        this.elements.currentMode = document.getElementById('currentMode');
        this.elements.modeButtons = document.querySelectorAll('.mode-btn');


        this.elements.currentController = document.getElementById('currentController');
        this.elements.controllerButtons = document.querySelectorAll('.controller-btn');


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


        this.elements.lightVisualization = document.getElementById('lightVisualization');


        this.elements.debugPixelIndex = document.getElementById('debugPixelIndex');
        this.elements.debugColor = document.getElementById('debugColor');
        this.elements.debugColorHex = document.getElementById('debugColorHex');
        this.elements.debugBrightness = document.getElementById('debugBrightness');
        this.elements.brightnessValue = document.getElementById('brightnessValue');
        this.elements.applyDebugBtn = document.getElementById('applyDebugBtn');
        this.elements.clearDebugBtn = document.getElementById('clearDebugBtn');
        this.elements.debugStatus = document.getElementById('debugStatus');


        this.elements.mqttLog = document.getElementById('mqttLog');
        this.elements.clearLogBtn = document.getElementById('clearLogBtn');
        this.elements.autoScrollLog = document.getElementById('autoScrollLog');


        this.elements.usernameInput.value = 'ucfninn';
        console.log('[UI] Username set to: ucfninn');

        this.setupEventListeners();
    }


    setupEventListeners() {

        this.elements.debugColor.addEventListener('input', (e) => {
            this.elements.debugColorHex.value = e.target.value.toUpperCase();
        });

        this.elements.debugColorHex.addEventListener('input', (e) => {
            const hex = e.target.value;
            if (/^#[0-9A-F]{6}$/i.test(hex)) {
                this.elements.debugColor.value = hex;
            }
        });


        this.elements.debugBrightness.addEventListener('input', (e) => {
            this.elements.brightnessValue.textContent = e.target.value;
        });


        this.elements.clearLogBtn.addEventListener('click', () => {
            this.clearLog();
        });


        this.elements.controllerButtons.forEach(btn => {
            btn.addEventListener('click', (e) => {
                if (!btn.disabled) {
                    const controller = btn.dataset.controller;

                    if (this.onControllerSwitch) {
                        this.onControllerSwitch(controller);
                    }
                }
            });
        });
    }


    updateConnectionStatus(connected) {
        this.state.connected = connected;

        if (connected) {
            this.elements.statusIndicator.classList.add('connected');
            this.elements.statusText.textContent = 'Connected';
            this.elements.connectBtn.disabled = true;
            this.elements.disconnectBtn.disabled = false;
            this.elements.usernameInput.disabled = true;


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


    updateMode(mode) {
        console.log('[UI] updateMode called with:', mode);
        console.log('[UI] Mode type:', typeof mode);

        this.state.currentMode = mode.toLowerCase();
        console.log('[UI] Current mode set to:', this.state.currentMode);


        this.elements.currentMode.textContent = mode.toUpperCase();
        this.elements.currentMode.className = 'mode-badge ' + this.state.currentMode;
        console.log('[UI] ✓ Mode badge updated to:', mode.toUpperCase());


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


    updateController(controller) {
        console.log('[UI] updateController called with:', controller);

        this.state.currentController = controller.toLowerCase();


        this.elements.currentController.textContent = controller.toUpperCase();


        this.elements.controllerButtons.forEach(btn => {
            if (btn.dataset.controller === this.state.currentController) {
                btn.classList.add('active');
            } else {
                btn.classList.remove('active');
            }
        });


        if (this.state.currentController === 'luminaire') {
            this.state.pixelCount = 72;
        } else {

            const localCount = this.elements.infoLighterNumber?.textContent;
            this.state.pixelCount = parseInt(localCount) || 8;
        }

        this.updateVisualization();
        console.log('[UI] ✓ Controller updated to:', controller);
    }


    updateInfo(field, value) {
        console.log('[UI] updateInfo called - field:', field, 'value:', value);


        const elementKey = field;
        console.log('[UI] Looking for element:', elementKey);

        const element = this.elements[elementKey];

        if (element) {
            console.log('[UI] ✓ Element found, updating to:', value);
            element.textContent = value;


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


    updatePixelSelector() {
        this.elements.debugPixelIndex.innerHTML = '';
        for (let i = 0; i < this.state.pixelCount; i++) {
            const option = document.createElement('option');
            option.value = i;
            option.textContent = `Pixel ${i}`;
            this.elements.debugPixelIndex.appendChild(option);
        }
    }


    updateVisualization() {
        this.elements.lightVisualization.innerHTML = '';


        this.elements.lightVisualization.setAttribute('data-pixel-count', this.state.pixelCount);


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
            return;
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


                const color = this.getModeColor(this.state.currentMode);
                pixel.style.backgroundColor = color;
                pixel.style.color = color;


                const isWhite = this.isWhiteColor(color);
                if (isWhite) {
                    pixel.classList.add('white-light');
                } else {
                    pixel.style.boxShadow = `0 0 15px ${color}, 0 0 30px ${color}`;
                }


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


    updatePixelColor(index, color) {
        const pixel = this.elements.lightVisualization.querySelector(`[data-index="${index}"]`);
        if (pixel && this.state.lightOn) {
            pixel.style.backgroundColor = color;
            pixel.style.color = color;


            const isWhite = this.isWhiteColor(color);
            if (isWhite) {
                pixel.classList.add('white-light');

                pixel.style.boxShadow = '';
            } else {
                pixel.classList.remove('white-light');
                pixel.style.boxShadow = `0 0 20px ${color}, 0 0 40px ${color}, 0 0 60px ${color}`;
            }

            pixel.classList.remove('breathing');
            pixel.classList.add('debug-mode');
            console.log('[UI] ✓ Pixel', index, 'color updated to', color);
        }
    }


    updatePixelBrightness(index, brightness) {
        const pixel = this.elements.lightVisualization.querySelector(`[data-index="${index}"]`);
        if (pixel && this.state.lightOn) {
            const opacity = Math.max(0.2, brightness / 255);
            pixel.style.opacity = opacity;
            pixel.classList.add('debug-mode');
            console.log('[UI] ✓ Pixel', index, 'brightness updated to', brightness, '(opacity:', opacity, ')');
        }
    }


    getModeColor(mode) {
        const colors = {
            'timer': '#ff4444',
            'weather': '#44ff44',
            'idle': '#4444ff',
            'music': '#ffffff'
        };
        return colors[mode] || '#ffffff';
    }


    isWhiteColor(color) {
        if (!color) return false;
        const normalized = color.toLowerCase().replace(/\s/g, '');
        return normalized === '#ffffff' ||
            normalized === '#fff' ||
            normalized === 'white' ||
            normalized === 'rgb(255,255,255)';
    }


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


        if (this.elements.autoScrollLog.checked) {
            this.elements.mqttLog.scrollTop = this.elements.mqttLog.scrollHeight;
        }


        const maxLogs = 100;
        while (this.elements.mqttLog.children.length > maxLogs) {
            this.elements.mqttLog.removeChild(this.elements.mqttLog.firstChild);
        }
    }


    clearLog() {
        this.elements.mqttLog.innerHTML = '';
    }


    getUsername() {
        return this.elements.usernameInput.value.trim();
    }


    getDebugSettings() {
        return {
            index: this.elements.debugPixelIndex.value,
            color: this.elements.debugColor.value,
            brightness: this.elements.debugBrightness.value
        };
    }


    saveUsername(username) {
        localStorage.setItem('auralight_username', username);
    }
}

export default new UIManager();
