#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

// --- CONFIGURATION ---

// WiFi Credentials (To be filled by user)
const char* ssid = "WIFI_SSID";
const char* password = "WIFI_PASSWORD";

// MQTT Broker
const char* mqtt_server = "captain.dev0.pandor.cloud";
const int mqtt_port = 1884;
const char* mqtt_topic = "station/meteo/data";

// Hardware Pinout
#define DHTPIN 4        // DHT22 Data Pin
#define DHTTYPE DHT22   // DHT 22 (AM2302)
#define BUTTON_PIN 14   // Push Button Pin (with internal pullup or external resistor)
#define LED_C_PIN 12    // LED for Celsius Mode
#define LED_F_PIN 13    // LED for Fahrenheit Mode

// --- GLOBALS ---

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

bool isFahrenheit = false;      // Default to Celsius
bool lastButtonState = HIGH;    // Input Pullup default
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
unsigned long lastMsgTime = 0;
const long interval = 2000;     // Publish every 2 seconds

// --- SETUP ---

void setup() {
  Serial.begin(115200);
  
  // Hardware Init
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Assumes button connects to GND when pressed
  pinMode(LED_C_PIN, OUTPUT);
  pinMode(LED_F_PIN, OUTPUT);
  
  dht.begin();
  
  // WiFi Init
  setup_wifi();
  
  // MQTT Init
  client.setServer(mqtt_server, mqtt_port);
  
  // Initial LED State
  updateLEDs();
}

// --- LOOP ---

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  handleButton();
  
  unsigned long now = millis();
  if (now - lastMsgTime > interval) {
    lastMsgTime = now;
    publishData();
  }
}

// --- FUNCTIONS ---

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void handleButton() {
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // If the state has changed (assumes LOW is pressed for INPUT_PULLUP)
    // We want to toggle on PRESS (HIGH -> LOW transition)
    static int buttonState = HIGH;
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        isFahrenheit = !isFahrenheit;
        updateLEDs();
        Serial.print("Unit switched to: ");
        Serial.println(isFahrenheit ? "Fahrenheit" : "Celsius");
        // Force immediate publish on change
        publishData(); 
      }
    }
  }
  lastButtonState = reading;
}

void updateLEDs() {
  if (isFahrenheit) {
    digitalWrite(LED_C_PIN, LOW);
    digitalWrite(LED_F_PIN, HIGH);
  } else {
    digitalWrite(LED_C_PIN, HIGH);
    digitalWrite(LED_F_PIN, LOW);
  }
}

void publishData() {
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // Read temp in Celsius (default)

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // NOTE: The web interface expects Celsius in the 'temperature' field for consistent logic.
  // The 'unit' field tells the receiver what mode the device is in physically.
  // However, if we want the DEVICE to send converted values:
  // float tempToSend = isFahrenheit ? dht.readTemperature(true) : t;
  
  // Requirement says: "Affiche les données sur une interface web en temps réel".
  // Usually, raw data (Celsius) is best for the cloud/web to handle conversion, 
  // BUT the LEDs indicate the mode ON THE DEVICE.
  // Let's send the raw Celsius value + the current Unit status so the web can sync if needed.
  
  StaticJsonDocument<200> doc;
  doc["temperature"] = t; // Always send Celsius base for consistency
  doc["humidity"] = h;
  doc["unit"] = isFahrenheit ? "F" : "C";
  doc["device_mode"] = "real"; // vs "simulation"

  char buffer[256];
  serializeJson(doc, buffer);

  client.publish(mqtt_topic, buffer);
  
  Serial.print("Published: ");
  Serial.println(buffer);
}
