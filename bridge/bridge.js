const WebSocket = require("ws");
const mqtt = require('mqtt');
const client = mqtt.connect("mqtt://captain.dev0.pandor.cloud:1884");


const webSocketClients = [];
client.on("connect", () => {
    client.subscribe("classroom/YounesBenaggoun", (err) => {
        if (!err) {
            console.log("subscribed to topic !");
        }
    });
});


client.on("message", (topic, message) => {
    const telemetry = JSON.parse(message.toString());
    console.log(telemetry);

    webSocketClients.forEach((ws) => {
        ws.send(JSON.stringify(telemetry));
    });
});

const wss = new WebSocket.Server({ port: 8080 });

wss.on("connection", (ws) => {
    console.log("Client Connected");
    webSocketClients.push(ws);
});

console.log("WebSocket Server on ws://localhost:8080");