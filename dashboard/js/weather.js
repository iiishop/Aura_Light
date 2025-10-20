/**
 * Weather Manager
 * 获取并显示 wttr.in 天气数据
 */

class WeatherManager {
    constructor() {
        this.city = 'London'; // 默认城市
        this.updateInterval = 600000; // 10分钟更新一次
        this.timer = null;
        
        // 天气代码到emoji的映射
        this.weatherIcons = {
            '113': '☀️',  // Sunny
            '116': '⛅',  // Partly cloudy
            '119': '☁️',  // Cloudy
            '122': '☁️',  // Overcast
            '143': '🌫️',  // Mist
            '176': '🌦️',  // Patchy rain possible
            '179': '🌨️',  // Patchy snow possible
            '182': '🌨️',  // Patchy sleet possible
            '185': '🌨️',  // Patchy freezing drizzle possible
            '200': '⛈️',  // Thundery outbreaks possible
            '227': '🌨️',  // Blowing snow
            '230': '❄️',  // Blizzard
            '248': '🌫️',  // Fog
            '260': '🌫️',  // Freezing fog
            '263': '🌦️',  // Patchy light drizzle
            '266': '🌧️',  // Light drizzle
            '281': '🌨️',  // Freezing drizzle
            '284': '🌨️',  // Heavy freezing drizzle
            '293': '🌦️',  // Patchy light rain
            '296': '🌧️',  // Light rain
            '299': '🌧️',  // Moderate rain at times
            '302': '🌧️',  // Moderate rain
            '305': '🌧️',  // Heavy rain at times
            '308': '🌧️',  // Heavy rain
            '311': '🌨️',  // Light freezing rain
            '314': '🌨️',  // Moderate or heavy freezing rain
            '317': '🌨️',  // Light sleet
            '320': '🌨️',  // Moderate or heavy sleet
            '323': '🌨️',  // Patchy light snow
            '326': '🌨️',  // Light snow
            '329': '🌨️',  // Patchy moderate snow
            '332': '❄️',  // Moderate snow
            '335': '❄️',  // Patchy heavy snow
            '338': '❄️',  // Heavy snow
            '350': '🌨️',  // Ice pellets
            '353': '🌦️',  // Light rain shower
            '356': '🌧️',  // Moderate or heavy rain shower
            '359': '🌧️',  // Torrential rain shower
            '362': '🌨️',  // Light sleet showers
            '365': '🌨️',  // Moderate or heavy sleet showers
            '368': '🌨️',  // Light snow showers
            '371': '❄️',  // Moderate or heavy snow showers
            '374': '🌨️',  // Light showers of ice pellets
            '377': '🌨️',  // Moderate or heavy showers of ice pellets
            '386': '⛈️',  // Patchy light rain with thunder
            '389': '⛈️',  // Moderate or heavy rain with thunder
            '392': '⛈️',  // Patchy light snow with thunder
            '395': '⛈️'   // Moderate or heavy snow with thunder
        };
    }

    /**
     * 设置城市
     */
    setCity(city) {
        this.city = city || 'London';
        console.log('[Weather] City set to:', this.city);
    }

    /**
     * 开始自动更新
     */
    startAutoUpdate() {
        console.log('[Weather] Starting auto-update...');
        this.fetchWeather();
        
        if (this.timer) {
            clearInterval(this.timer);
        }
        
        this.timer = setInterval(() => {
            this.fetchWeather();
        }, this.updateInterval);
    }

    /**
     * 停止自动更新
     */
    stopAutoUpdate() {
        if (this.timer) {
            clearInterval(this.timer);
            this.timer = null;
        }
    }

    /**
     * 获取天气数据
     */
    async fetchWeather() {
        try {
            console.log(`[Weather] Fetching weather for ${this.city}...`);
            
            // 使用 wttr.in API
            const url = `https://wttr.in/${encodeURIComponent(this.city)}?format=j1`;
            
            const response = await fetch(url);
            
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }
            
            const data = await response.json();
            console.log('[Weather] Data received:', data);
            
            if (data.current_condition && data.current_condition.length > 0) {
                this.displayWeather(data.current_condition[0]);
            } else {
                this.showError('No weather data available');
            }
            
        } catch (error) {
            console.error('[Weather] Fetch error:', error);
            this.showError('Failed to load weather data');
        }
    }

    /**
     * 显示天气数据
     */
    displayWeather(condition) {
        console.log('[Weather] Displaying weather:', condition);
        
        // 温度
        const tempValue = document.getElementById('tempValue');
        if (tempValue) {
            tempValue.textContent = condition.temp_C;
        }
        
        // 天气描述
        const weatherDesc = document.getElementById('weatherDesc');
        if (weatherDesc) {
            // 优先使用中文描述
            if (condition.lang_zh && condition.lang_zh.length > 0) {
                weatherDesc.textContent = condition.lang_zh[0].value;
            } else if (condition.weatherDesc && condition.weatherDesc.length > 0) {
                weatherDesc.textContent = condition.weatherDesc[0].value;
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
            const iconCode = condition.weatherCode;
            const emoji = this.weatherIcons[iconCode] || '🌡️';
            weatherIcon.textContent = emoji;
            weatherIcon.className = 'weather-icon';
            
            // 添加天气类型class
            if (['113'].includes(iconCode)) {
                weatherIcon.classList.add('sunny');
            } else if (['116', '119', '122'].includes(iconCode)) {
                weatherIcon.classList.add('cloudy');
            } else if (iconCode.startsWith('2') || iconCode.startsWith('3')) {
                weatherIcon.classList.add('rainy');
            }
        }
        
        // 湿度
        const humidity = document.getElementById('humidity');
        if (humidity) {
            humidity.textContent = `${condition.humidity}%`;
        }
        
        // 风速
        const windSpeed = document.getElementById('windSpeed');
        if (windSpeed) {
            windSpeed.textContent = `${condition.windspeedKmph} km/h ${condition.winddir16Point}`;
        }
        
        // 能见度
        const visibility = document.getElementById('visibility');
        if (visibility) {
            visibility.textContent = `${condition.visibility} km`;
        }
        
        // 体感温度
        const feelsLike = document.getElementById('feelsLike');
        if (feelsLike) {
            feelsLike.textContent = `${condition.FeelsLikeC}°C`;
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
