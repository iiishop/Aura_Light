/**
 * MQTT配置模块
 * 管理MQTT连接配置
 */

export const MQTT_CONFIG = {
    broker: 'mqtt.cetools.org',
    port: 8090,  // MQTT TCP port
    username: 'student',
    password: 'ce2021-mqtt-forget-whale',

    // Topic模板
    getTopicBase: (username) => `student/CASA0014/${username}`,

    // 具体topic
    topics: {
        status: '/status',
        mode: '/mode',
        debugColor: '/debug/color',
        debugBrightness: '/debug/brightness',
        debugIndex: '/debug/index',

        // INFO topics
        infoWifiSSID: '/info/wifi/ssid',
        infoWifiIP: '/info/wifi/ip',
        infoWifiRSSI: '/info/wifi/rssi',
        infoWifiMAC: '/info/wifi/mac',
        infoLighterNumber: '/info/lighter/number',
        infoLighterPin: '/info/lighter/pin',
        infoSystemVersion: '/info/system/version',
        infoSystemUptime: '/info/system/uptime',
        infoLocationCity: '/info/location/city'
    },

    // 获取完整topic路径
    getFullTopic: (username, topicSuffix) => {
        return `${MQTT_CONFIG.getTopicBase(username)}${topicSuffix}`;
    },

    // 获取所有订阅topic
    getAllSubscribeTopics: (username) => {
        const base = MQTT_CONFIG.getTopicBase(username);
        return [
            `${base}/status`,
            `${base}/mode`,
            `${base}/debug/#`,
            `${base}/info/#`
        ];
    }
};

export default MQTT_CONFIG;
