const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const mqtt = require('mqtt');
const path = require('path');
const cors = require('cors');

const app = express();
app.use(cors());
app.use(express.static(path.join(__dirname, 'public')));

const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// Configuration MQTT
const MQTT_BROKER = 'mqtt://captain.dev0.pandor.cloud:1884';
const MQTT_TOPIC = 'station/meteo/data'; // Topic par défaut

console.log(`Connecting to MQTT broker at ${MQTT_BROKER}...`);
const mqttClient = mqtt.connect(MQTT_BROKER);

mqttClient.on('connect', () => {
    console.log('Connected to MQTT Broker');
    mqttClient.subscribe('#', (err) => { // S'abonne à tout pour le debug, ou spécifique
        if (!err) {
            console.log(`Subscribed to all topics (#) for debugging`);
        }
    });
});

mqttClient.on('message', (topic, message) => {
    // console.log(`Received message on ${topic}: ${message.toString()}`);
    
    // Broadcast to all connected WebSocket clients
    const payload = JSON.stringify({
        topic: topic,
        message: message.toString(),
        timestamp: new Date().toISOString()
    });

    wss.clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(payload);
        }
    });
});

mqttClient.on('error', (err) => {
    console.error('MQTT Error:', err);
});

// WebSocket connection handling
wss.on('connection', (ws) => {
    console.log('New WebSocket client connected');
    
    ws.send(JSON.stringify({
        type: 'info',
        message: 'Connected to WebSocket Bridge'
    }));

    ws.on('close', () => {
        console.log('Client disconnected');
    });
});

const PORT = process.env.PORT || 3001;
server.listen(PORT, () => {
    console.log(`Server running on http://localhost:${PORT}`);
});
