# Station Météo Connectée - Interface Web

Ce projet contient l'interface web (Frontend) et le pont WebSocket (Bridge) pour le projet de Station Météo Connectée (HETIC WEB3).

## Prérequis

- Node.js installé.

## Installation

1.  Ouvrez un terminal dans ce dossier.
2.  Installez les dépendances :
    ```bash
    npm install
    ```

## Lancement

1.  Lancez le serveur (Bridge + Frontend) :
    ```bash
    node server.js
    ```
2.  Ouvrez votre navigateur à l'adresse : `http://localhost:3001`

## Fonctionnalités

-   **Dashboard Temps Réel** : Affiche la température et l'humidité.
-   **Graphique** : Historique des données en temps réel.
-   **Pont MQTT-WebSocket** : Le fichier `server.js` se connecte au broker MQTT (`captain.dev0.pandor.cloud:1884`) et relaie les messages vers le frontend via WebSocket.
-   **Configuration Topic** : Vous pouvez changer le topic MQTT écouté directement depuis l'interface.
-   **Mode Simulation** : Un bouton permet de générer des données fictives pour tester l'interface sans capteur.

## Configuration MQTT

Le broker est configuré dans `server.js` :
```javascript
const MQTT_BROKER = 'mqtt://captain.dev0.pandor.cloud:1884';
```
Si vous devez changer de broker, modifiez cette ligne.

## Format des données attendu

Le frontend s'attend à recevoir des messages JSON sur le topic MQTT (ex: `station/meteo/data`) au format suivant :
```json
{
  "temperature": 24.5,
  "humidity": 60.0,
  "unit": "C"
}
```
- `unit` est optionnel (par défaut "C"), mais s'il est présent ("F"), l'affichage s'adapte.
