/**
 * MQTT管理器模块
 * 处理所有MQTT连接和消息
 */

import { MQTT_CONFIG } from './config.js';

class MQTTManager {
    constructor() {
        this.client = null;
        this.username = '';
        this.connected = false;
        this.callbacks = {
            onConnect: null,
            onDisconnect: null,
            onMessage: null,
            onError: null
        };
    }

    /**
     * 连接到MQTT broker
     */
    connect(username) {
        return new Promise((resolve, reject) => {
            this.username = username;

            // 使用 mqtt:// 协议（不是 ws://）
            const mqttUrl = `mqtt://${MQTT_CONFIG.broker}:${MQTT_CONFIG.port}`;

            console.log('=== MQTT Connection Debug ===');
            console.log('[MQTT] URL:', mqttUrl);
            console.log('[MQTT] Broker:', MQTT_CONFIG.broker);
            console.log('[MQTT] Port:', MQTT_CONFIG.port);
            console.log('[MQTT] Auth Username:', MQTT_CONFIG.username);
            console.log('[MQTT] Auth Password:', MQTT_CONFIG.password ? '***' + MQTT_CONFIG.password.slice(-5) : 'none');
            console.log('[MQTT] User:', username);
            console.log('[MQTT] Client ID:', `dashboard_${username}_${Date.now()}`);
            console.log('[MQTT] MQTT.js version:', typeof mqtt !== 'undefined' ? 'loaded' : 'NOT LOADED');
            console.log('============================');

            // 创建MQTT客户端
            try {
                const options = {
                    username: MQTT_CONFIG.username,
                    password: MQTT_CONFIG.password,
                    clientId: `dashboard_${username}_${Date.now()}`,
                    clean: true,
                    reconnectPeriod: 5000,
                    connectTimeout: 10000,
                    keepalive: 60,
                    protocolVersion: 4  // MQTT 3.1.1
                };

                console.log('[MQTT] Connection options:', JSON.stringify({ ...options, password: '***' }, null, 2));

                this.client = mqtt.connect(mqttUrl, options);
                console.log('[MQTT] Client created, waiting for events...');
            } catch (error) {
                console.error('[MQTT] !!! Connection creation failed !!!');
                console.error('[MQTT] Error type:', error.constructor.name);
                console.error('[MQTT] Error message:', error.message);
                console.error('[MQTT] Error stack:', error.stack);
                reject(error);
                return;
            }

            // 连接成功
            this.client.on('connect', (connack) => {
                console.log('[MQTT] ✓✓✓ Connected successfully! ✓✓✓');
                console.log('[MQTT] Session present:', connack.sessionPresent);
                console.log('[MQTT] Return code:', connack.returnCode);
                this.connected = true;

                // 订阅所有相关topic
                this.subscribeAll();

                if (this.callbacks.onConnect) {
                    this.callbacks.onConnect();
                }

                resolve();
            });

            // 接收消息
            this.client.on('message', (topic, message) => {
                const msg = message.toString();
                console.log(`[MQTT] ← Received: ${topic} = ${msg}`);

                if (this.callbacks.onMessage) {
                    this.callbacks.onMessage(topic, msg);
                }
            });

            // 连接错误
            this.client.on('error', (error) => {
                console.error('[MQTT] !!! ERROR EVENT !!!');
                console.error('[MQTT] Error type:', error.constructor.name);
                console.error('[MQTT] Error message:', error.message);
                console.error('[MQTT] Error code:', error.code);
                console.error('[MQTT] Full error:', error);
                if (this.callbacks.onError) {
                    this.callbacks.onError(error);
                }
                reject(error);
            });

            // 断开连接
            this.client.on('close', () => {
                console.log('[MQTT] ✗✗✗ Connection CLOSED ✗✗✗');
                console.log('[MQTT] Was connected:', this.connected);
                this.connected = false;
                if (this.callbacks.onDisconnect) {
                    this.callbacks.onDisconnect();
                }
            });

            // 离线事件
            this.client.on('offline', () => {
                console.log('[MQTT] ⚠️ Client is OFFLINE');
            });

            // 重连事件
            this.client.on('reconnect', () => {
                console.log('[MQTT] 🔄 Attempting to RECONNECT...');
            });

            // 数据包接收
            this.client.on('packetsend', (packet) => {
                console.log('[MQTT] → Packet sent:', packet.cmd, packet);
            });

            this.client.on('packetreceive', (packet) => {
                console.log('[MQTT] ← Packet received:', packet.cmd, packet);
            });
        });
    }

    /**
     * 订阅所有topic
     */
    subscribeAll() {
        const topics = MQTT_CONFIG.getAllSubscribeTopics(this.username);

        console.log('[MQTT] Subscribing to topics:', topics);

        topics.forEach(topic => {
            this.client.subscribe(topic, (err) => {
                if (err) {
                    console.error(`[MQTT] ✗ Subscribe failed: ${topic}`, err);
                } else {
                    console.log(`[MQTT] ✓ Subscribed successfully: ${topic}`);
                }
            });
        });

        // 订阅完成后，请求设备发送INFO数据
        console.log('[MQTT] Requesting device to publish INFO...');
        setTimeout(() => {
            // 发布一个请求消息让设备重新发送INFO
            // 如果设备支持 /request topic的话
            const requestTopic = MQTT_CONFIG.getFullTopic(this.username, '/request');
            this.client.publish(requestTopic, 'info', (err) => {
                if (err) {
                    console.warn('[MQTT] Request INFO failed (device may not support this)');
                } else {
                    console.log('[MQTT] INFO request sent');
                }
            });
        }, 1000);
    }

    /**
     * 断开连接
     */
    disconnect() {
        if (this.client) {
            this.client.end();
            this.connected = false;
        }
    }

    /**
     * 发布消息
     */
    publish(topicSuffix, message) {
        if (!this.connected) {
            console.error('[MQTT] Not connected');
            return false;
        }

        const fullTopic = MQTT_CONFIG.getFullTopic(this.username, topicSuffix);
        this.client.publish(fullTopic, message.toString(), (err) => {
            if (err) {
                console.error(`[MQTT] Publish failed: ${fullTopic}`, err);
            } else {
                console.log(`[MQTT] Published: ${fullTopic} = ${message}`);
            }
        });

        return true;
    }

    /**
     * 设置回调函数
     */
    on(event, callback) {
        if (this.callbacks.hasOwnProperty(`on${event.charAt(0).toUpperCase()}${event.slice(1)}`)) {
            this.callbacks[`on${event.charAt(0).toUpperCase()}${event.slice(1)}`] = callback;
        }
    }

    /**
     * 检查是否已连接
     */
    isConnected() {
        return this.connected;
    }
}

export default new MQTTManager();
