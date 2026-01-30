#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Infos de connection Wifi

const char *WIFI_SSID = "s22ultraYounes";
const char *WIFI_PASS = "12341234";

// Infos MQTT

const char *MQTT_HOST = "captain.dev0.pandor.cloud";
const uint16_t MQTT_PORT = 1884;
const char *MQTT_TOPIC = "classroom/YounesBenaggoun";

// Si on avait de l'auth sur le MQTT
const char *MQTT_USER = "";
const char *MQTT_PASS = "";

const char *DEVICE_ID = "esp32-Younes";
uint32_t seq = 42;
const uint32_t baseTs = 1767828437;
const uint32_t publishIntervalMs = 5000;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long lastPublishMs = 0;

/////////////////
///////////////

const int Button_pin = 2;
bool currentButtonState = 0;
bool lastButtonState = 0;

const int red_led = 5;
const int green_led = 4;
bool green_led_state = 0;

int temperaturePin = 22;

// float humidity = 0;
float temperature = 0;
float finaleTemperature = 0;

bool degreeMode = 0;
//////////////////////////
////////////////////////////

void connectWiFi()
{
    Serial.print("[WIFI] Connecting to "); // Serial.print => Pas de retour de ligne
    Serial.println(WIFI_SSID);             // Serial.println => Retour de ligne

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("OK.");

    Serial.print("WiFi: connected, IP=");
    Serial.println(WiFi.localIP());
}

void connectMQTT()
{
    mqtt.setServer(MQTT_HOST, MQTT_PORT);

    while (!mqtt.connected())
    {
        String clientId = String("esp32-") + String((uint32_t)ESP.getEfuseMac(), HEX);

        Serial.print("MQTT: connecting as ");
        Serial.println(clientId);

        bool ok;

        ok = mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS);

        if (ok)
        {
            Serial.println("[MQTT] Connected");
        }
        else
        {
            Serial.print("[MQTT] failed, rc=");
            Serial.print(mqtt.state());
            Serial.println(" retry in 2s");
            delay(2000);
        }
    }
}

void publishDummyTelemetry(float tempC, float humPct, bool degreeMode)
{
    // float tempC = 22.4f + (float)random(-15,16) / 10.0f;
    // float humPct = 42.67f + (float)random(-30,31) / 10.0f;
    int batteryPct = 100;

    StaticJsonDocument<256> doc;
    JsonObject t = doc.to<JsonObject>();
    t["deviceId"] = DEVICE_ID;
    t["ts"] = baseTs + (millis() / 1000);
    t["seq"] = seq++;
    t["tempC"] = tempC;
    t["humPct"] = humPct;
    t["degreeMode"] = degreeMode;
    t["batteryPct"] = batteryPct;

    char payload[256];
    size_t n = serializeJson(doc, payload, sizeof(payload));

    bool ok = mqtt.publish(MQTT_TOPIC, payload, n);
    Serial.print("[MQTT] Publish to ");
    Serial.print(MQTT_TOPIC);
    Serial.print(" ... ");
    Serial.println(ok ? payload : "FAILED");
}
///////////////////////////////////////
////////////////////////////////////////

float fahrenheitToCelsius(float fahren)
{
    return (fahren - 32) / 1.8;
}

float getTemp()
{
    float calculatedTemp = 0;

    temperature = analogRead(temperaturePin);

    currentButtonState = digitalRead(Button_pin);

    if (currentButtonState != lastButtonState)
    {
        lastButtonState = currentButtonState;
        if (currentButtonState == 0)
        {
            // Serial.println("keyDown ");
        }
        else
        {
            // Serial.println("KeyUp ");
            if (green_led_state)
            {
                digitalWrite(green_led, HIGH);
                digitalWrite(red_led, LOW);
                green_led_state = 0;
                degreeMode = 1;

                // Serial.println(finaleTemperature);
            }
            else
            {
                digitalWrite(green_led, LOW);
                digitalWrite(red_led, HIGH);
                green_led_state = 1;
                degreeMode = 0;
            }
        }
    }

    if (degreeMode)
    {
        calculatedTemp = fahrenheitToCelsius(temperature);
    }
    else
    {
        calculatedTemp = temperature;
    }
    return calculatedTemp;
}
/////////////////////////////////////
/////////////////////////////////

void setup()
{
    Serial.begin(115200);
    delay(1000);

    connectWiFi();
    connectMQTT();
    // put your setup code here, to run once:

    /////////////////
    pinMode(Button_pin, INPUT_PULLUP);
    pinMode(temperaturePin, INPUT);
    pinMode(humidityPin, INPUT);
    ////////////////
}

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        connectWiFi();
    }
    if (!mqtt.connected())
    {
        connectMQTT();
    }

    mqtt.loop();

    unsigned long now = millis();

    ////////////////////////////////////////

    humidity = analogRead(humidityPin);
    finaleTemperature = getTemp();
    ///////////////////////////////////////////

    if (now - lastPublishMs >= publishIntervalMs)
    {

        lastPublishMs = now;
        publishDummyTelemetry(finaleTemperature, humidity, degreeMode);
    }
}