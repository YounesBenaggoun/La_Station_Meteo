/**
 * Station météo ESP32 - HETIC
 * - Mode simulation (sans DHT22) ou mode réel (avec DHT22)
 * - Bascule °C / °F par bouton physique (avec anti-rebond)
 * - LEDs d'état d'unité
 * - Publication MQTT en JSON
 * - Réception commande d'unité via MQTT (bonus)
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// --------- CONFIG GÉNÉRALE ---------

// Passe à false quand tu as le vrai capteur branché
#define SIMULATION_MODE true

// WiFi
const char* ssid = "Ton_SSID";
const char* password = "Ton_MDP";

// MQTT
const char* mqtt_server = "captain.dev0.pandor.cloud";
const int   mqtt_port   = 1884;

// Topics MQTT
const char* TOPIC_DATA      = "hetic/weather/data";
const char* TOPIC_UNIT_SET  = "hetic/weather/unit/set";

// GPIO
#define BUTTON_PIN 13
#define LED_C_PIN  26
#define LED_F_PIN  27

// DHT22 (mode réel)
#define DHTPIN  4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

bool  unitCelsius   = true;
float temperature   = 0.0;
float humidity      = 0.0;

// Anti-rebond bouton
unsigned long lastDebounceTime   = 0;
bool          lastButtonState    = HIGH;
const unsigned long debounceDelay = 200; // ms

// Gestion fréquence de mesure / publication
unsigned long lastMeasureTime     = 0;
const unsigned long measurePeriod = 2000; // ms

// ---------- WIFI / MQTT ----------

void updateLEDs() {
  digitalWrite(LED_C_PIN, unitCelsius ? HIGH : LOW);
  digitalWrite(LED_F_PIN, unitCelsius ? LOW  : HIGH);
}

void setupWifi() {
  delay(100);
  Serial.print("Connexion au WiFi ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi connecté !");
  Serial.print("IP : ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Gestion des commandes entrantes (bonus)
  if (strcmp(topic, TOPIC_UNIT_SET) == 0 && length > 0) {
    char cmd = (char)payload[0];
    if (cmd == 'C') unitCelsius = true;
    if (cmd == 'F') unitCelsius = false;
    updateLEDs();

    Serial.print("Commande unité reçue via MQTT : ");
    Serial.println(unitCelsius ? "°C" : "°F");
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Connexion MQTT...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("✅ MQTT connecté !");
      client.subscribe(TOPIC_UNIT_SET);
    } else {
      Serial.print("Échec, rc=");
      Serial.print(client.state());
      Serial.println(" nouvelle tentative dans 2s");
      delay(2000);
    }
  }
}

// ---------- CAPTEUR / SIMULATION ----------

void readSensor() {
  if (SIMULATION_MODE) {
    // Génération de valeurs réalistes pour la simulation
    temperature = random(150, 300) / 10.0; // 15.0°C à 30.0°C
    humidity    = random(300, 800) / 10.0; // 30.0% à 80.0%
  } else {
    // Lecture réelle DHT22
    float t = dht.readTemperature();    // °C
    float h = dht.readHumidity();       // %

    if (isnan(t) || isnan(h)) {
      Serial.println("⚠️  Erreur lecture DHT22");
      return; // on garde les anciennes valeurs
    }

    temperature = t;
    humidity    = h;
  }
}

void publishData() {
  StaticJsonDocument<256> doc;

  float tempToSend = unitCelsius
    ? temperature
    : (temperature * 9.0 / 5.0) + 32.0;

  doc["temperature"] = tempToSend;
  doc["humidity"]    = humidity;
  doc["unit"]        = unitCelsius ? "C" : "F";
  doc["simulation"]  = SIMULATION_MODE;

  char buffer[256];
  size_t len = serializeJson(doc, buffer);

  client.publish(TOPIC_DATA, buffer, len);

  Serial.print("MQTT → ");
  serializeJson(doc, Serial);
  Serial.println();
}

// ---------- BOUTON / UNITÉ ----------

void handleButton() {
  bool reading = digitalRead(BUTTON_PIN);

  // Changement d'état → on (re)lance le timer de debounce
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  // Une fois le délai passé, on valide le changement
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Front descendant : bouton pressé (INPUT_PULLUP)
    if (reading == LOW && lastButtonState == HIGH) {
      unitCelsius = !unitCelsius;
      updateLEDs();

      Serial.print("Unité changée par le bouton : ");
      Serial.println(unitCelsius ? "°C" : "°F");

      // Optionnel : publier aussi la nouvelle unité sur MQTT
      char u = unitCelsius ? 'C' : 'F';
      client.publish(TOPIC_UNIT_SET, &u, 1);
    }
  }

  lastButtonState = reading;
}

// ---------- CYCLE PRINCIPAL ----------

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_C_PIN, OUTPUT);
  pinMode(LED_F_PIN, OUTPUT);

  updateLEDs();

  if (!SIMULATION_MODE) {
    dht.begin();
    Serial.println("DHT22 initialisé (mode réel)");
  } else {
    Serial.println("Mode simulation actif (sans DHT22)");
  }

  setupWifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  handleButton();

  unsigned long now = millis();
  if (now - lastMeasureTime >= measurePeriod) {
    lastMeasureTime = now;
    readSensor();
    publishData();
  }
}
