

class WeatherManager {
    constructor() {
        this.city = 'London'; 

        
        this.weatherIcons = {
            '113': '☀',  
            '116': '⛅',  
            '119': '☁',  
            '122': '☁',  
            '143': '🌫',  
            '176': '🌦',  
            '179': '🌨',  
            '182': '🌨',  
            '185': '🌨',  
            '200': '⛈',  
            '227': '🌨',  
            '230': '❄',  
            '248': '🌫',  
            '260': '🌫',  
            '263': '🌦',  
            '266': '🌧',  
            '281': '🌨',  
            '284': '🌨',  
            '293': '🌦',  
            '296': '🌧',  
            '299': '🌧',  
            '302': '🌧',  
            '305': '🌧',  
            '308': '🌧',  
            '311': '🌨',  
            '314': '🌨',  
            '317': '🌨',  
            '320': '🌨',  
            '323': '🌨',  
            '326': '🌨',  
            '329': '🌨',  
            '332': '❄',  
            '335': '❄',  
            '338': '❄',  
            '350': '🌨',  
            '353': '🌦',  
            '356': '🌧',  
            '359': '🌧',  
            '362': '🌨',  
            '365': '🌨',  
            '368': '🌨',  
            '371': '❄',  
            '374': '🌨',  
            '377': '🌨',  
            '386': '⛈',  
            '389': '⛈',  
            '392': '⛈',  
            '395': '⛈'   
        };
    }

    
    setCity(city) {
        this.city = city || 'London';
        console.log('[Weather] City set to:', this.city);
    }

    
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

    
    displayWeather(data) {
        console.log('[Weather] Displaying weather:', data);

        
        const tempValue = document.getElementById('tempValue');
        if (tempValue) {
            tempValue.textContent = data.temp_C || '--';
        }

        
        const weatherDesc = document.getElementById('weatherDesc');
        if (weatherDesc) {
            
            if (data.weatherDesc_zh) {
                weatherDesc.textContent = data.weatherDesc_zh;
            } else {
                weatherDesc.textContent = data.weatherDesc || '--';
            }
        }

        
        const weatherLocation = document.getElementById('weatherLocation');
        if (weatherLocation) {
            weatherLocation.textContent = this.city;
        }

        
        const weatherIcon = document.getElementById('weatherIcon');
        if (weatherIcon) {
            const iconCode = data.weatherCode;
            const icon = this.weatherIcons[iconCode] || '🌡';
            weatherIcon.textContent = icon;
            weatherIcon.className = 'weather-icon';

            
            if (['113'].includes(iconCode)) {
                weatherIcon.classList.add('sunny');
            } else if (['116', '119', '122'].includes(iconCode)) {
                weatherIcon.classList.add('cloudy');
            } else if (iconCode && (iconCode.startsWith('2') || iconCode.startsWith('3'))) {
                weatherIcon.classList.add('rainy');
            }
        }

        
        const humidity = document.getElementById('humidity');
        if (humidity) {
            humidity.textContent = data.humidity ? `${data.humidity}%` : '--%';
        }

        
        const windSpeed = document.getElementById('windSpeed');
        if (windSpeed) {
            if (data.windspeedKmph && data.winddir16Point) {
                windSpeed.textContent = `${data.windspeedKmph} km/h ${data.winddir16Point}`;
            } else {
                windSpeed.textContent = '--';
            }
        }

        
        const visibility = document.getElementById('visibility');
        if (visibility) {
            visibility.textContent = data.visibility ? `${data.visibility} km` : '--';
        }

        
        const feelsLike = document.getElementById('feelsLike');
        if (feelsLike) {
            feelsLike.textContent = data.FeelsLikeC ? `${data.FeelsLikeC}°C` : '--';
        }
    }

    
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

export default WeatherManager;
