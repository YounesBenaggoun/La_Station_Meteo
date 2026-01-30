// Configuration
const MQTT_BROKER = 'ws://captain.dev0.pandor.cloud:1884';
const MQTT_TOPIC = 'hetic/groupe1/meteo'; // Remplacez 'groupe1' par votre numéro de groupe

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
// Background Layers
const bgLayers = {
    cold: document.querySelector('.bg-layer.bg-cold'),
    mild: document.querySelector('.bg-layer.bg-mild'),
    warm: document.querySelector('.bg-layer.bg-warm'),
    hot: document.querySelector('.bg-layer.bg-hot')
};
const body = document.body;

// State
let client = null;
let simulationInterval = null;
let isSimulationMode = false;
let currentTempC = 0; // Store last known temp in Celsius (default base)
let isFahrenheit = false;

// --- Logic ---

function connectMQTT() {
    console.log(`Connecting to MQTT broker: ${MQTT_BROKER}...`);
    client = mqtt.connect(MQTT_BROKER);

    client.on('connect', () => {
        console.log('SYS', 'MQTT_CONNECTED');
        mainCondition.textContent = "Connecté (MQTT)";
        
        client.subscribe(MQTT_TOPIC, (err) => {
            if (!err) {
                console.log(`Subscribed to topic: ${MQTT_TOPIC}`);
            } else {
                console.error('Subscription error:', err);
            }
        });
    });

    client.on('message', (topic, message) => {
        if (isSimulationMode) return;
        
        console.log(`[Received] ${topic}:`, message.toString());

        try {
            const payload = JSON.parse(message.toString());
            
            // Expected JSON: {"temp": float, "hum": float, "unit": "C" or "F"}
            updateDashboard(payload);

        } catch (e) {
            console.error('JSON Parsing Error:', e);
        }
    });

    client.on('error', (err) => {
        console.error('MQTT Error:', err);
        mainCondition.textContent = "Erreur MQTT";
    });

    client.on('offline', () => {
        console.log('MQTT Offline');
        mainCondition.textContent = "Déconnecté...";
    });
}

function updateDashboard(data) {
    // Handle Temperature (support 'temp' or 'temperature' for compatibility)
    const tempVal = data.temp !== undefined ? data.temp : data.temperature;
    
    if (tempVal !== undefined) {
        // Handle Unit Update from Device
        if (data.unit) {
            const newUnitIsF = (data.unit === 'F');
            if (isFahrenheit !== newUnitIsF) {
                isFahrenheit = newUnitIsF;
                updateUnitButtonUI();
            }
        }

        // If data comes in F, convert to C for internal storage/logic if needed, 
        // OR just store what we have. 
        // Logic: The app stores `currentTempC` as base.
        // If device sends F, we convert to C for base storage.
        if (isFahrenheit) {
            currentTempC = (tempVal - 32) * 5/9;
        } else {
            currentTempC = tempVal;
        }

        renderTemperature();
        
        // Update Dynamic Background (always based on Celsius)
        updateBackground(currentTempC);
    }
    
    // Handle Humidity (support 'hum' or 'humidity')
    const humVal = data.hum !== undefined ? data.hum : data.humidity;
    
    if (humVal !== undefined) {
        if (humidityVal) humidityVal.textContent = `${humVal.toFixed(0)} %`;
        if (tempVal !== undefined) updateConditionText(currentTempC, humVal);
    }
}

function renderTemperature() {
    let displayTemp = currentTempC;
    let unitSymbol = "°";

    if (isFahrenheit) {
        displayTemp = (currentTempC * 9/5) + 32;
        unitSymbol = "°F";
    }

    const tempStr = displayTemp.toFixed(1) + "°"; // Show 1 decimal for precision
    
    // Update Main Display
    mainTemp.textContent = tempStr;
    sidebarTemp.textContent = tempStr;
    hourlyNow.textContent = tempStr;

    // Update High/Low (Mock logic: +/- 3 degrees for visual)
    const high = isFahrenheit ? ((currentTempC + 3) * 9/5 + 32) : (currentTempC + 3);
    const low = isFahrenheit ? ((currentTempC - 2) * 9/5 + 32) : (currentTempC - 2);

    maxTempEl.textContent = `H: ${high.toFixed(0)}°`;
    minTempEl.textContent = `B: ${low.toFixed(0)}°`;
    
    // Update Feels Like (Mock calculation: Temp - 2 for wind chill effect)
    const feelsLikeC = currentTempC - 2;
    const feelsLikeDisplay = isFahrenheit ? (feelsLikeC * 9/5 + 32) : feelsLikeC;
    
    if (feelsLikeVal) feelsLikeVal.textContent = `${feelsLikeDisplay.toFixed(0)}°`;
}

function updateUnitButtonUI() {
    unitToggleBtn.textContent = isFahrenheit ? "Basculer en °C" : "Basculer en °F";
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
    // Reset all layers first? No, we just want to set the active one.
    // CSS opacity transition handles the cross-fade.
    
    // 1. Determine which layer should be active
    let activeKey = 'mild'; // default
    
    if (temp < 10) {
        activeKey = 'cold';
    } else if (temp >= 10 && temp < 20) {
        activeKey = 'mild';
    } else if (temp >= 20 && temp < 30) {
        activeKey = 'warm';
    } else {
        activeKey = 'hot';
    }

    // 2. Apply 'active' class to the target, remove from others
    Object.keys(bgLayers).forEach(key => {
        if (key === activeKey) {
            bgLayers[key].classList.add('active');
        } else {
            bgLayers[key].classList.remove('active');
        }
    });
}

function updateTime() {
    const now = new Date();
    sidebarTime.textContent = now.toLocaleTimeString('fr-FR', { hour: '2-digit', minute: '2-digit' });
}
setInterval(updateTime, 1000);
updateTime();

// Controls
unitToggleBtn.addEventListener('click', () => {
    isFahrenheit = !isFahrenheit;
    updateUnitButtonUI();
    renderTemperature();
});

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

            // Simulate incoming data format
            // Don't send unit in simulation to allow manual toggle testing
            updateDashboard({ temp: simTemp, hum: simHum });
        }, 200);
    } else {
        simBtn.textContent = 'Mode Simulation';
        simBtn.style.background = 'rgba(0,0,0,0.4)';
        clearInterval(simulationInterval);
        mainCondition.textContent = "En attente...";
    }
});

// Start MQTT Connection
connectMQTT();
