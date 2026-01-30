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
const uint32_t publishIntervalMs = 5000;
unsigned long lastPublishMs = 0;
uint32_t seq = 1;

/* ================= HARDWARE ================= */
const int BUTTON_PIN = 2;
const int LED_RED    = 5;   // Fahrenheit
const int LED_GREEN  = 4;   // Celsius

const int TEMP_PIN     = 22; // simulation
const int HUMIDITY_PIN = 23; // simulation

bool degreeModeCelsius = true;
bool lastButtonState = HIGH;

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

/* ================= UTILS ================= */
float fahrenheitToCelsius(float f) {
  return (f - 32.0) / 1.8;
}

/* ================= BUTTON / LED ================= */
void handleButton() {
  bool currentState = digitalRead(BUTTON_PIN);

  if (currentState == LOW && lastButtonState == HIGH) {
    degreeModeCelsius = !degreeModeCelsius;

    digitalWrite(LED_GREEN, degreeModeCelsius ? HIGH : LOW);
    digitalWrite(LED_RED,   degreeModeCelsius ? LOW  : HIGH);
  }
  lastButtonState = currentState;
}

/* ================= MQTT PUBLISH ================= */
void publishTelemetry(float temp, float hum) {
  StaticJsonDocument<256> doc;

  doc["deviceId"] = DEVICE_ID;
  doc["seq"] = seq++;
  doc["temperature"] = temp;
  doc["humidity"] = hum;
  doc["unit"] = degreeModeCelsius ? "C" : "F";

  char payload[256];
  serializeJson(doc, payload);

  mqtt.publish(MQTT_TOPIC, payload);
  Serial.print("[MQTT] Published: ");
  Serial.println(payload);
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);

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
  if (now - lastPublishMs >= publishIntervalMs) {
    lastPublishMs = now;

    // ===== MODE SIMULATION =====
    float temp = random(180, 300) / 10.0; // 18.0 → 30.0
    float hum  = random(300, 700) / 10.0; // 30 → 70 %

    if (!degreeModeCelsius) {
      temp = temp * 1.8 + 32; // Celsius → Fahrenheit
    }

    publishTelemetry(temp, hum);
  }
}
