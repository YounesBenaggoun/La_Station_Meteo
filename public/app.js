// Configuration
const WS_URL = `ws://${window.location.hostname}:${window.location.port}`;
let currentTopic = 'station/meteo/data';

// DOM Elements
const mainTemp = document.getElementById('main-temp');
const mainCondition = document.getElementById('main-condition');
const maxTempEl = document.getElementById('max-temp');
const minTempEl = document.getElementById('min-temp');
const sidebarTemp = document.getElementById('sidebar-temp');
const sidebarTime = document.getElementById('sidebar-time');
const hourlyNow = document.getElementById('hourly-now');

const feelsLikeVal = document.getElementById('feels-like-val');
const humidityVal = document.getElementById('humidity-val');
const simBtn = document.getElementById('sim-btn');
const unitToggleBtn = document.getElementById('unit-toggle');
const body = document.body;

// State
let isConnected = false;
let simulationInterval = null;
let isSimulationMode = false;
let currentTempC = 0; // Store last known temp in Celsius
let isFahrenheit = false;

// --- Logic ---

function connectWebSocket() {
    const ws = new WebSocket(WS_URL);

    ws.onopen = () => {
        isConnected = true;
        console.log('SYS', 'UPLINK_ESTABLISHED');
        mainCondition.textContent = "Connecté";
    };

    ws.onmessage = (event) => {
        if (isSimulationMode) return;
        try {
            const data = JSON.parse(event.data);
            if (data.type === 'info') return;
            if (data.topic === currentTopic) {
                const payload = JSON.parse(data.message);
                updateDashboard(payload);
            }
        } catch (e) {
            console.error(e);
        }
    };

    ws.onclose = () => {
        isConnected = false;
        console.log('ERR', 'CONNECTION_LOST. RETRYING...');
        mainCondition.textContent = "Déconnecté...";
        setTimeout(connectWebSocket, 5000);
    };
}

function updateDashboard(data) {
    if (data.temperature !== undefined) {
        currentTempC = data.temperature;
        renderTemperature();
        
        // Update Dynamic Background (always based on Celsius)
        updateBackground(currentTempC);
        updateConditionText(currentTempC, data.humidity);
    }
    
    if (data.humidity !== undefined) {
        if (humidityVal) humidityVal.textContent = `${data.humidity.toFixed(0)} %`;
    }
}

function renderTemperature() {
    let displayTemp = currentTempC;
    let unit = "°"; // or °C

    if (isFahrenheit) {
        displayTemp = (currentTempC * 9/5) + 32;
        unit = "°F";
    }

    const tempStr = displayTemp.toFixed(0) + "°";
    
    // Update Main Display
    mainTemp.textContent = tempStr;
    sidebarTemp.textContent = tempStr;
    hourlyNow.textContent = tempStr;

    // Update High/Low (Mock logic: +/- 3 degrees for visual)
    // We calculate these based on the current unit
    const high = isFahrenheit ? ((currentTempC + 3) * 9/5 + 32) : (currentTempC + 3);
    const low = isFahrenheit ? ((currentTempC - 2) * 9/5 + 32) : (currentTempC - 2);

    maxTempEl.textContent = `H: ${high.toFixed(0)}°`;
    minTempEl.textContent = `B: ${low.toFixed(0)}°`;
    
    // Update Feels Like (Mock calculation: Temp - 2 for wind chill effect)
    const feelsLikeC = currentTempC - 2;
    const feelsLikeDisplay = isFahrenheit ? (feelsLikeC * 9/5 + 32) : feelsLikeC;
    
    if (feelsLikeVal) feelsLikeVal.textContent = `${feelsLikeDisplay.toFixed(0)}°`;
}

function updateConditionText(temp, hum) {
    // Simple logic to determine condition text
    let condition = "Nuageux";
    if (temp > 25 && hum < 40) condition = "Ensoleillé";
    else if (hum > 80) condition = "Pluvieux";
    else if (temp < 5) condition = "Glacial";
    else if (hum < 30) condition = "Ciel dégagé";
    
    mainCondition.textContent = condition;
}

function updateBackground(temp) {
    body.classList.remove('bg-cold', 'bg-mild', 'bg-warm', 'bg-hot');
    
    if (temp < 10) {
        body.classList.add('bg-cold');
    } else if (temp >= 10 && temp < 20) {
        body.classList.add('bg-mild');
    } else if (temp >= 20 && temp < 30) {
        body.classList.add('bg-warm');
    } else {
        body.classList.add('bg-hot');
    }
}

function updateTime() {
    const now = new Date();
    sidebarTime.textContent = now.toLocaleTimeString('fr-FR', { hour: '2-digit', minute: '2-digit' });
}
setInterval(updateTime, 1000);
updateTime();

// Controls
simBtn.addEventListener('click', () => {
    isSimulationMode = !isSimulationMode;
    
    if (isSimulationMode) {
        simBtn.textContent = 'Mode Simulation: ON';
        simBtn.style.background = 'rgba(255, 255, 255, 0.2)';
        
        let simTemp = 15;
        let simHum = 50;
        let direction = 1;

        simulationInterval = setInterval(() => {
            // Smooth sine wave simulation for temp to show color transitions
            simTemp += 0.2 * direction;
            if (simTemp > 35) direction = -1;
            if (simTemp < 5) direction = 1;
            
            simHum += (Math.random() - 0.5) * 2;
            if (simHum < 0) simHum = 0; if (simHum > 100) simHum = 100;

            updateDashboard({ temperature: simTemp, humidity: simHum });
        }, 200); // Fast updates to show smooth transition
    } else {
        simBtn.textContent = 'Mode Simulation';
        simBtn.style.background = 'rgba(0,0,0,0.4)';
        clearInterval(simulationInterval);
        mainCondition.textContent = "En attente...";
    }
});

// Unit Toggle
unitToggleBtn.addEventListener('click', () => {
    isFahrenheit = !isFahrenheit;
    unitToggleBtn.textContent = isFahrenheit ? "Basculer en °C" : "Basculer en °F";
    renderTemperature();
});

connectWebSocket();
