/*
 * Bridge MQTT ↔ WebSocket
 * Récupère les données MQTT et les envoie aux clients WebSocket
 */

const express = require('express');
const WebSocket = require('ws');
const mqtt = require('mqtt');
const http = require('http');
const path = require('path');

// config
const MQTT_BROKER = 'mqtt://captain.dev0.pandor.cloud:1884';
const MQTT_TOPIC_DATA = 'station/meteo/data';
const MQTT_TOPIC_COMMAND = 'station/meteo/command';
const WS_PORT = 3000;

const app = express();
const server = http.createServer(app);

app.use(express.json());
app.use(express.static(path.join(__dirname, '../frontend')));

// websocket
const wss = new WebSocket.Server({ server });
const clients = new Set();

wss.on('connection', (ws) => {
  console.log(`✓ Client WebSocket connecté. Total: ${wss.clients.size}`);
  clients.add(ws);
  
  ws.on('message', (message) => {
    try {
      const data = JSON.parse(message);
      console.log('[WS → MQTT]', data);
      
      if (data.unit) {
        mqttClient.publish(MQTT_TOPIC_COMMAND, JSON.stringify(data));
      }
    } catch (e) {
      console.error('Erreur parsing WS message:', e.message);
    }
  });
  
  ws.on('close', () => {
    clients.delete(ws);
    console.log(`✗ Client déconnecté. Total: ${wss.clients.size}`);
  });
  
  ws.on('error', (error) => {
    console.error('WebSocket error:', error.message);
  });
});

// mqqt client
const mqttClient = mqtt.connect(MQTT_BROKER, {
  clean: true,
  reconnectPeriod: 1000,
});

mqttClient.on('connect', () => {
  console.log('✓ Connecté au broker MQTT');
  mqttClient.subscribe(MQTT_TOPIC_DATA, (err) => {
    if (err) {
      console.error('Erreur subscription:', err);
    } else {
      console.log(`✓ Subscription: ${MQTT_TOPIC_DATA}`);
    }
  });
});

mqttClient.on('message', (topic, message) => {
  try {
    const data = JSON.parse(message.toString());
    console.log(`[MQTT → WS] ${topic}:`, data);
    
    const payload = JSON.stringify({
      type: 'data',
      ...data,
      receivedAt: new Date().toISOString()
    });
    
    wss.clients.forEach((client) => {
      if (client.readyState === WebSocket.OPEN) {
        client.send(payload);
      }
    });
  } catch (e) {
    console.error('Erreur parsing MQTT message:', e.message);
  }
});

mqttClient.on('error', (error) => {
  console.error('MQTT error:', error.message);
});

mqttClient.on('offline', () => {
  console.log('✗ MQTT client offline');
});

app.get('/api/status', (req, res) => {
  res.json({
    mqtt: mqttClient.connected ? 'connected' : 'disconnected',
    clients: wss.clients.size,
    broker: MQTT_BROKER,
    topics: {
      data: MQTT_TOPIC_DATA,
      command: MQTT_TOPIC_COMMAND
    }
  });
});

app.post('/api/command', express.json(), (req, res) => {
  const { unit } = req.body;
  
  if (!unit || (unit !== 'C' && unit !== 'F')) {
    return res.status(400).json({ error: 'Invalid unit (C or F)' });
  }
  
  mqttClient.publish(MQTT_TOPIC_COMMAND, JSON.stringify({ unit }));
  
  res.json({ success: true, unit });
});

server.listen(WS_PORT, () => {
  console.log(`Serveur running: http://localhost:${WS_PORT}`);
  console.log(`WebSocket: ws://localhost:${WS_PORT}`);
  console.log(`MQTT Broker: ${MQTT_BROKER}\n`);
});


process.on('SIGINT', () => {
  console.log('\nFermeture...');
  mqttClient.end();
  server.close(() => {
    console.log('Server fermé');
    process.exit(0);
  });
});
