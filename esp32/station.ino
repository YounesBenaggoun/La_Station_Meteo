/*
 * Station Météo Connectée ESP32
 * Donc ici j'ai fait le capteur DHT22 MQTT
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

// config simul
#define SIMULATION_MODE true
#define DHTPIN 4
#define DHTTYPE DHT22

// pins
#define BUTTON_PIN 5
#define LED_CELSIUS_PIN 18
#define LED_FAHRENHEIT_PIN 19
#define PUBLISH_INTERVAL 5000

// Ici on gere la wifi et mqtt
const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";
const char *mqtt_server = "captain.dev0.pandor.cloud";
const int mqtt_port = 1884;
const char *mqtt_topic_data = "station/meteo/data";
const char *mqtt_topic_command = "station/meteo/command";

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastPublishTime = 0;
unsigned long lastButtonCheckTime = 0;
bool buttonPressed = false;
bool isCelsius = true;

float simTemp = 22.5;
float simHumidity = 55.0;
bool simTempIncreasing = true;

void setupWiFi();
void setupMQTT();
void reconnect();
void onMessageReceived(char *topic, byte *payload, unsigned int length);
void updateIndicatorLEDs();
void handleButtonPress();
void publishData();
void simulateData();
float celsiusToFahrenheit(float celsius);

void setup()
{
    Serial.begin(115200);
    delay(100);

    Serial.println("\n\n=== Station Météo Démarrage ===");

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_CELSIUS_PIN, OUTPUT);
    pinMode(LED_FAHRENHEIT_PIN, OUTPUT);

    // Ici j'initialise la dht
    if (!SIMULATION_MODE)
    {
        dht.begin();
        Serial.println("Capteur DHT22 initialisé");
    }
    else
    {
        Serial.println("MODE SIMULATION ACTIVÉ");
    }

    updateIndicatorLEDs();

    // Ici je gere la conexion wifi
    setupWiFi();
    setupMQTT();

    Serial.println("=== Démarrage Terminé ===\n");
}

void loop()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();

    handleButtonPress();

    // Ici je gere les données peridodiques
    unsigned long currentTime = millis();
    if (currentTime - lastPublishTime >= PUBLISH_INTERVAL)
    {
        publishData();
        lastPublishTime = currentTime;
    }

    delay(50);
}

void setupWiFi()
{
    delay(10);
    Serial.print("Connexion WiFi : ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    Serial.println();
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("WiFi connecté");
        Serial.print("IP : ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("Erreur WiFi - Continuant quand même...");
    }
}

void setupMQTT()
{
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(onMessageReceived);
    Serial.println("Broker MQTT : " + String(mqtt_server) + ":" + String(mqtt_port));
}

void reconnect()
{
    static unsigned long lastReconnectAttempt = 0;
    unsigned long now = millis();

    if (now - lastReconnectAttempt < 5000)
    {
        return;
    }
    lastReconnectAttempt = now;

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi déconnecté");
        return;
    }

    Serial.print("Connexion MQTT...");
    if (client.connect("ESP32-Station"))
    {
        Serial.println(" OK");
        client.subscribe(mqtt_topic_command);
        Serial.println("Subscription : " + String(mqtt_topic_command));
    }
    else
    {
        Serial.print(" Erreur (code: ");
        Serial.print(client.state());
        Serial.println(")");
    }
}

void onMessageReceived(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message reçu [");
    Serial.print(topic);
    Serial.println("]");

    char msg[length + 1];
    for (unsigned int i = 0; i < length; i++)
    {
        msg[i] = (char)payload[i];
    }
    msg[length] = '\0';

    Serial.print("Contenu : ");
    Serial.println(msg);

    // Ici je gere le parser JSON
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, msg);

    if (error)
    {
        Serial.println("Erreur JSON");
        return;
    }

    if (doc.containsKey("unit"))
    {
        const char *unit = doc["unit"];
        if (strcmp(unit, "C") == 0)
        {
            isCelsius = true;
            Serial.println("Unité : Celsius");
        }
        else if (strcmp(unit, "F") == 0)
        {
            isCelsius = false;
            Serial.println("Unité : Fahrenheit");
        }
        updateIndicatorLEDs();
    }
}

// ici je gere le bouton avec debounce
void handleButtonPress()
{
    unsigned long currentTime = millis();

    if (currentTime - lastButtonCheckTime < 200)
    {
        return;
    }
    lastButtonCheckTime = currentTime;

    if (digitalRead(BUTTON_PIN) == LOW)
    {
        if (!buttonPressed)
        {
            buttonPressed = true;

            isCelsius = !isCelsius;
            Serial.println(isCelsius ? "Basculé vers Celsius" : "Basculé vers Fahrenheit");

            updateIndicatorLEDs();

            StaticJsonDocument<100> doc;
            doc["unit"] = isCelsius ? "C" : "F";
            char buffer[100];
            serializeJson(doc, buffer);
            client.publish(mqtt_topic_command, buffer);
        }
    }
    else
    {
        buttonPressed = false;
    }
}

void updateIndicatorLEDs()
{
    if (isCelsius)
    {
        digitalWrite(LED_CELSIUS_PIN, HIGH);
        digitalWrite(LED_FAHRENHEIT_PIN, LOW);
    }
    else
    {
        digitalWrite(LED_CELSIUS_PIN, LOW);
        digitalWrite(LED_FAHRENHEIT_PIN, HIGH);
    }
}

void publishData()
{
    float temp, humidity;

    if (SIMULATION_MODE)
    {
        simulateData();
        temp = simTemp;
        humidity = simHumidity;
    }
    else
    {
        humidity = dht.readHumidity();
        temp = dht.readTemperature();

        if (isnan(humidity) || isnan(temp))
        {
            Serial.println("Erreur capteur DHT22");
            return;
        }
    }

    float displayTemp = isCelsius ? temp : celsiusToFahrenheit(temp);
    const char *unit = isCelsius ? "C" : "F";

    StaticJsonDocument<256> doc;
    doc["temperature"] = round(displayTemp * 10.0) / 10.0; // 1 décimale
    doc["humidity"] = round(humidity * 10.0) / 10.0;
    doc["unit"] = unit;
    doc["timestamp"] = millis();
    doc["simulation"] = SIMULATION_MODE;

    char buffer[256];
    serializeJson(doc, buffer);

    if (client.publish(mqtt_topic_data, buffer))
    {
        Serial.print(" Published: ");
        Serial.println(buffer);
    }
    else
    {
        Serial.println(" Publish failed");
    }
}

void simulateData()
{
    simTemp += simTempIncreasing ? 0.3 : -0.3;

    if (simTemp >= 28.0)
        simTempIncreasing = false;
    if (simTemp <= 18.0)
        simTempIncreasing = true;

    simHumidity = 45.0 + (sinf(millis() / 10000.0) + 1.0) * 15.0;
}

float celsiusToFahrenheit(float celsius)
{
    return (celsius * 9.0 / 5.0) + 32.0;
}
