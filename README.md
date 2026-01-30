
<<<<<<< HEAD
1- Code TinkerCad avec (un capteur de température et capteur d'humidité , deux led , et un button ) 
2- ajouter le code wifi avec les accées du wifi du téléphone de l'un notre équipe. 
3- bridge en node js (pour connecté au serveur mqTT du prof ) et un serveur websocket pour connecté avec le html 
4- front End simple avec un code html et fichier main.js avec le webSocket et l'affichage du Json (affichage F pour fahrenheit et ° pour degree selcus )
=======
# Station Météo Connectée — HETIC WEB3

Projet réalisé dans le cadre du TP Station Météo Connectée à HETIC.

L’objectif est de concevoir une station météo basée sur un ESP32, capable de publier des données de température et d’humidité via le protocole MQTT, puis de les exploiter en temps réel.

---

## Objectifs

- Publier des données météo via MQTT
- Gérer un mode simulation (capteurs partagés)
- Changer l’unité de température (Celsius / Fahrenheit)
- Indiquer l’unité active à l’aide de LEDs
- Utiliser une architecture modulaire et temps réel

---

## Architecture

ESP32 → Broker MQTT → Bridge Node.js → Interface Web

- L’ESP32 publie les données météo
- Le broker MQTT transporte les messages
- Le bridge Node.js s’abonne aux topics MQTT
- Le frontend consomme les données

---

## Technologies utilisées

### Hardware
- ESP32
- Bouton poussoir
- 2 LEDs
- Résistances
- Capteur DHT22 (tests finaux)

### Software
- Arduino
- MQTT
- Mosquitto
- Node.js
- PubSubClient
- ArduinoJson
- mqtt.js

---

## Structure du projet

station-meteo/
├── esp32/
│   └── station.ino
├── bridge/
│   ├── index.js
│   └── package.json
├── docs/
│   ├── photos/
│   ├── tinkercad/
│   └── schema-cablage.png
├── config/
│   └── SETUP.md
├── .gitignore
└── README.md


---

## ESP32

### Fonctionnalités
- Connexion WiFi
- Connexion au broker MQTT
- Publication périodique des données
- Mode simulation
- Bouton pour basculer Celsius / Fahrenheit
- LEDs indiquant l’unité active

### Format des messages MQTT

json
{
  "deviceId": "esp32-Younes",
  "seq": 1,
  "temperature": 22.5,
  "humidity": 48.2,
  "unit": "C"
}


### Topic MQTT


classroom/YounesBenaggoun


---

## MQTT

### Broker mutualisé

* Host : captain.dev0.pandor.cloud
* Port : 1884
* Authentification : aucune

### Broker local (tests)

* Mosquitto
* Port : 1883

Le broker local a été utilisé pour valider le fonctionnement lorsque le broker mutualisé était indisponible.

---

## Bridge Node.js

Le bridge permet de s’abonner aux topics MQTT et de recevoir les messages JSON.

### Lancement

bash
cd bridge
npm install
node index.js


---

## Tests

* Mode simulation testé sans capteur
* Publication MQTT validée avec `mosquitto_pub`
* Abonnement MQTT validé avec `mosquitto_sub`
* Intégration complète validée en local

---
YOUNES BENAGGOUN
RAYANE MOUHAJER
DUMAS JOLLY AXEL
DANALI MABANZA
KATIA SAKRI 

>>>>>>> cb6c1c40acd19ac3d5b228d193685a5fa0e47ba8
