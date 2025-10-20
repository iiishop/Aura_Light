/**
 * Weather Manager
 * 从 MQTT broker 订阅并显示天气数据
 */

class WeatherManager {
    constructor() {
        this.city = 'London'; // 默认城市

        // 天气代码到图标的映射（文字描述）
        this.weatherIcons = {
            '113': '☀',  // Sunny
            '116': '⛅',  // Partly cloudy
            '119': '☁',  // Cloudy
            '122': '☁',  // Overcast
            '143': '🌫',  // Mist
            '176': '🌦',  // Patchy rain possible
            '179': '🌨',  // Patchy snow possible
            '182': '🌨',  // Patchy sleet possible
            '185': '🌨',  // Patchy freezing drizzle possible
            '200': '⛈',  // Thundery outbreaks possible
            '227': '🌨',  // Blowing snow
            '230': '❄',  // Blizzard
            '248': '🌫',  // Fog
            '260': '🌫',  // Freezing fog
            '263': '🌦',  // Patchy light drizzle
            '266': '🌧',  // Light drizzle
            '281': '🌨',  // Freezing drizzle
            '284': '🌨',  // Heavy freezing drizzle
            '293': '🌦',  // Patchy light rain
            '296': '🌧',  // Light rain
            '299': '🌧',  // Moderate rain at times
            '302': '🌧',  // Moderate rain
            '305': '🌧',  // Heavy rain at times
            '308': '🌧',  // Heavy rain
            '311': '🌨',  // Light freezing rain
            '314': '🌨',  // Moderate or heavy freezing rain
            '317': '🌨',  // Light sleet
            '320': '🌨',  // Moderate or heavy sleet
            '323': '🌨',  // Patchy light snow
            '326': '🌨',  // Light snow
            '329': '🌨',  // Patchy moderate snow
            '332': '❄',  // Moderate snow
            '335': '❄',  // Patchy heavy snow
            '338': '❄',  // Heavy snow
            '350': '🌨',  // Ice pellets
            '353': '🌦',  // Light rain shower
            '356': '🌧',  // Moderate or heavy rain shower
            '359': '🌧',  // Torrential rain shower
            '362': '🌨',  // Light sleet showers
            '365': '🌨',  // Moderate or heavy sleet showers
            '368': '🌨',  // Light snow showers
            '371': '❄',  // Moderate or heavy snow showers
            '374': '🌨',  // Light showers of ice pellets
            '377': '🌨',  // Moderate or heavy showers of ice pellets
            '386': '⛈',  // Patchy light rain with thunder
            '389': '⛈',  // Moderate or heavy rain with thunder
            '392': '⛈',  // Patchy light snow with thunder
            '395': '⛈'   // Moderate or heavy snow with thunder
        };
    }

    /**
     * 设置城市（仅用于显示）
     */
    setCity(city) {
        this.city = city || 'London';
        console.log('[Weather] City set to:', this.city);
    }

    /**
     * 处理从MQTT接收到的天气数据
     */
    handleWeatherData(jsonString) {
        try {
            const data = JSON.parse(jsonString);
            console.log('[Weather] Received weather data:', data);
            this.displayWeather(data);
        } catch (error) {
            console.error('[Weather] Parse error:', error);
            this.showError('Invalid weather data');
        }
    }

    /**
     * 显示天气数据
     */
    displayWeather(data) {
        console.log('[Weather] Displaying weather:', data);

        // 温度
        const tempValue = document.getElementById('tempValue');
        if (tempValue) {
            tempValue.textContent = data.temp_C || '--';
        }

        // 天气描述
        const weatherDesc = document.getElementById('weatherDesc');
        if (weatherDesc) {
            // 优先使用中文描述
            if (data.weatherDesc_zh) {
                weatherDesc.textContent = data.weatherDesc_zh;
            } else {
                weatherDesc.textContent = data.weatherDesc || '--';
            }
        }

        // 城市
        const weatherLocation = document.getElementById('weatherLocation');
        if (weatherLocation) {
            weatherLocation.textContent = this.city;
        }

        // 天气图标
        const weatherIcon = document.getElementById('weatherIcon');
        if (weatherIcon) {
            const iconCode = data.weatherCode;
            const icon = this.weatherIcons[iconCode] || '🌡';
            weatherIcon.textContent = icon;
            weatherIcon.className = 'weather-icon';

            // 添加天气类型class
            if (['113'].includes(iconCode)) {
                weatherIcon.classList.add('sunny');
            } else if (['116', '119', '122'].includes(iconCode)) {
                weatherIcon.classList.add('cloudy');
            } else if (iconCode && (iconCode.startsWith('2') || iconCode.startsWith('3'))) {
                weatherIcon.classList.add('rainy');
            }
        }

        // 湿度
        const humidity = document.getElementById('humidity');
        if (humidity) {
            humidity.textContent = data.humidity ? `${data.humidity}%` : '--%';
        }

        // 风速
        const windSpeed = document.getElementById('windSpeed');
        if (windSpeed) {
            if (data.windspeedKmph && data.winddir16Point) {
                windSpeed.textContent = `${data.windspeedKmph} km/h ${data.winddir16Point}`;
            } else {
                windSpeed.textContent = '--';
            }
        }

        // 能见度
        const visibility = document.getElementById('visibility');
        if (visibility) {
            visibility.textContent = data.visibility ? `${data.visibility} km` : '--';
        }

        // 体感温度
        const feelsLike = document.getElementById('feelsLike');
        if (feelsLike) {
            feelsLike.textContent = data.FeelsLikeC ? `${data.FeelsLikeC}°C` : '--';
        }
    }

    /**
     * 显示错误
     */
    showError(message) {
        console.error('[Weather] Error:', message);

        const weatherIcon = document.getElementById('weatherIcon');
        if (weatherIcon) {
            weatherIcon.innerHTML = '<div class="weather-loading">⚠️ Error</div>';
        }

        const weatherDesc = document.getElementById('weatherDesc');
        if (weatherDesc) {
            weatherDesc.textContent = message;
        }
    }
}

// 导出
export default WeatherManager;
