

export const MQTT_CONFIG = {
    broker: 'mqtt.cetools.org',
    port: 8090,  
    username: 'student',
    password: 'ce2021-mqtt-forget-whale',

    
    getTopicBase: (username) => `student/CASA0014/${username}`,

    
    topics: {
        status: '/status',
        mode: '/mode',
        controller: '/controller',
        debugColor: '/debug/color',
        debugBrightness: '/debug/brightness',
        debugIndex: '/debug/index',

        
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

    
    getFullTopic: (username, topicSuffix) => {
        return `${MQTT_CONFIG.getTopicBase(username)}${topicSuffix}`;
    },

    
    getAllSubscribeTopics: (username) => {
        const base = MQTT_CONFIG.getTopicBase(username);
        return [
            `${base}/status`,
            `${base}/mode`,
            `${base}/controller`,
            `${base}/debug/#`,
            `${base}/info/#`
        ];
    }
};

export default MQTT_CONFIG;
