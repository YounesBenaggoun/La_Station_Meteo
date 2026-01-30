#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

/* ================= WIFI ================= */
const char *WIFI_SSID = "s22ultraYounes";
const char *WIFI_PASS = "12341234";

/* ================= MQTT ================= */
const char *MQTT_HOST  = "captain.dev0.pandor.cloud";
const uint16_t MQTT_PORT = 1884;
const char *MQTT_TOPIC = "classroom/YounesBenaggoun";

const char *MQTT_USER = "";
const char *MQTT_PASS = "";
const char *DEVICE_ID = "esp32-Younes";

/* ================= TIMING ================= */
const uint32_t PUBLISH_INTERVAL_MS = 5000;
unsigned long lastPublishMs = 0;
uint32_t seq = 1;

/* ================= HARDWARE ================= */
const int BUTTON_PIN = 2;
const int LED_GREEN  = 4;   // Celsius
const int LED_RED    = 5;   // Fahrenheit

/* Simulation pins (pas de DHT en mode simulation) */
const int TEMP_PIN     = 22;
const int HUMIDITY_PIN = 23;

/* ================= STATES ================= */
bool degreeModeCelsius = true;
bool lastButtonState = HIGH;

/* Anti-rebond */
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

/* ================= CLIENTS ================= */
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

/* ================= WIFI ================= */
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("[WIFI] Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" OK");
  Serial.print("[WIFI] IP: ");
  Serial.println(WiFi.localIP());
}

/* ================= MQTT ================= */
void connectMQTT() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);

  while (!mqtt.connected()) {
    String clientId = "esp32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    Serial.print("[MQTT] Connecting as ");
    Serial.println(clientId);

    if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println("[MQTT] Connected");
    } else {
      Serial.print("[MQTT] Failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" retry in 2s");
      delay(2000);
    }
  }
}

/* ================= BUTTON / LED ================= */
void handleButton() {
  bool currentState = digitalRead(BUTTON_PIN);

  if (currentState == LOW && lastButtonState == HIGH) {
    if (millis() - lastDebounceTime > debounceDelay) {
      degreeModeCelsius = !degreeModeCelsius;

      digitalWrite(LED_GREEN, degreeModeCelsius ? HIGH : LOW);
      digitalWrite(LED_RED,   degreeModeCelsius ? LOW  : HIGH);

      lastDebounceTime = millis();
    }
  }
  lastButtonState = currentState;
}

/* ================= MQTT PUBLISH ================= */
void publishTelemetry(float temperature, float humidity) {
  StaticJsonDocument<256> doc;

  doc["deviceId"] = DEVICE_ID;
  doc["seq"] = seq++;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["unit"] = degreeModeCelsius ? "C" : "F";

  char payload[256];
  serializeJson(doc, payload);

  bool ok = mqtt.publish(MQTT_TOPIC, payload);
  Serial.print("[MQTT] Publish: ");
  Serial.println(ok ? payload : "FAILED");
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  /* État initial : Celsius */
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);

  randomSeed(analogRead(0));

  connectWiFi();
  connectMQTT();
}

/* ================= LOOP ================= */
void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  handleButton();

  unsigned long now = millis();
  if (now - lastPublishMs >= PUBLISH_INTERVAL_MS) {
    lastPublishMs = now;

    /* ===== MODE SIMULATION ===== */
    float tempC = random(180, 300) / 10.0; // 18.0 → 30.0 °C
    float humidity = random(300, 700) / 10.0; // 30 → 70 %

    float tempToSend = tempC;
    if (!degreeModeCelsius) {
      tempToSend = tempC * 1.8 + 32.0; // °F
    }

    publishTelemetry(tempToSend, humidity);
  }
}

