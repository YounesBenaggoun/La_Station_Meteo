const mqtt = require("mqtt");

const MQTT_BROKER = "mqtt://localhost:1883";
const MQTT_TOPIC = "hetic/weather/data";

console.log("Tentative de connexion MQTT…");

const client = mqtt.connect(MQTT_BROKER, {
  connectTimeout: 5000,
  reconnectPeriod: 0 // pas de retry infini
});

client.on("connect", () => {
  console.log("MQTT connecté au broker");
  client.subscribe(MQTT_TOPIC);
});

client.on("message", (topic, message) => {
  console.log("MQTT >", message.toString());
});

client.on("error", (err) => {
  console.error("MQTT ERROR :", err.message);
});

client.on("close", () => {
  console.log("MQTT connexion fermée");
});

client.on("offline", () => {
  console.log("MQTT hors ligne");
});
