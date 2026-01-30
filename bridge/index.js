const mqtt = require("mqtt");
const WebSocket = require("ws");

// ---------- CONFIG ----------
const MQTT_URL        = "mqtt://captain.dev0.pandor.cloud:1884";
const TOPIC_DATA      = "hetic/weather/data";
const TOPIC_UNIT_SET  = "hetic/weather/unit/set";
const WS_PORT         = 8080;

// ---------- MQTT ----------
const mqttClient = mqtt.connect(MQTT_URL);

// ---------- WebSocket ----------
const wss = new WebSocket.Server({ port: WS_PORT });

// Quand le bridge démarre
console.log(`🚀 Bridge MQTT ↔ WebSocket`);
console.log(`   MQTT : ${MQTT_URL}`);
console.log(`   WS   : ws://localhost:${WS_PORT}`);

// Connexion MQTT
mqttClient.on("connect", () => {
  console.log("✅ MQTT connecté");
  mqttClient.subscribe(TOPIC_DATA, (err) => {
    if (err) {
      console.error("Erreur subscribe MQTT :", err);
    } else {
      console.log(`📡 Abonné au topic ${TOPIC_DATA}`);
    }
  });
});

// Réception d'un message MQTT (capteur → frontend)
mqttClient.on("message", (topic, message) => {
  if (topic !== TOPIC_DATA) return;

  const payload = message.toString();

  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(payload);
    }
  });
});

// Connexion d'un client WebSocket (frontend)
wss.on("connection", (socket) => {
  console.log("🌐 Client WebSocket connecté");

  // Réception d'un message du frontend (commande → MQTT)
  socket.on("message", (raw) => {
    try {
      const data = JSON.parse(raw.toString());

      // Exemple attendu : { type: "setUnit", unit: "C" }
      if (data.type === "setUnit" && (data.unit === "C" || data.unit === "F")) {
        mqttClient.publish(TOPIC_UNIT_SET, data.unit);
        console.log(`➡️  Commande unité envoyée au MQTT : ${data.unit}`);
      }
    } catch (e) {
      console.error("Message WebSocket invalide :", e);
    }
  });

  socket.on("close", () => {
    console.log("❌ Client WebSocket déconnecté");
  });
});
