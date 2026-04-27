/*
  ESP32 MQTT Publisher  -  DHT11 + Potentiometer
  -----------------------------------------------
  Publishes three values to broker.hivemq.com (port 1883) every second:

    Topic        Payload         Source
    -----------  --------------  -----------------------------
    TEMPERATURE  e.g. "24.30"    DHT11 (GPIO4)
    HUMIDITY    e.g. "58.00"    DHT11 (GPIO4)
    VOLTAGE      e.g. "1.652"    Potentiometer wiper (GPIO34)

  These match the topics already used by the Python subscriber
  (mqtt_subscribe.py), so no changes are needed on the consumer side.


  Wiring:
    DHT11   VCC -> 3V3        Data -> GPIO4    GND -> GND
    POT     left -> 3V3       wiper -> GPIO34  right -> GND

  Libraries (install via Library Manager or platformio.ini):
    - "PubSubClient"  by Nick O'Leary
    - "DHT sensor library" by Adafruit
    - "Adafruit Unified Sensor"   (dependency of the DHT library)
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ---------------------------------------------------------------------------
// User-editable settings  (change these to match your environment)
// ---------------------------------------------------------------------------
const char* WIFI_SSID     = "enter wifi name";
const char* WIFI_PASSWORD = "wnter password";

const char* MQTT_BROKER   = "broker.hivemq.com";
const uint16_t MQTT_PORT  = 1883;
const char* MQTT_CLIENT_ID = "ESP32_SmartFarm_Node01";  // must be unique on broker

// Pin assignments
const uint8_t PIN_DHT = 4;        // DHT11 data line
const uint8_t PIN_POT = 34;       // ADC1_CH6 - input-only, ADC-capable

// Timing
const uint32_t PUBLISH_INTERVAL_MS = 1000;  // publish once per second

// ADC reference - the ESP32 reads 0..4095 over a 0..3.3 V range (default attenuation)
const float ADC_VREF   = 3.3f;
const int   ADC_MAX    = 4095;

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
DHT dht(PIN_DHT, DHT11);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

uint32_t lastPublishMs = 0;

// ---------------------------------------------------------------------------
// Wi-Fi
// ---------------------------------------------------------------------------
void connectWiFi() {
  Serial.printf("Connecting to Wi-Fi '%s'", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nWi-Fi connected, IP = %s\n", WiFi.localIP().toString().c_str());
}

// ---------------------------------------------------------------------------
// MQTT
// ---------------------------------------------------------------------------
void connectMQTT() {
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);

  while (!mqtt.connected()) {
    Serial.printf("Connecting to MQTT broker %s ... ", MQTT_BROKER);
    if (mqtt.connect(MQTT_CLIENT_ID)) {
      Serial.println("connected");
    } else {
      Serial.printf("failed (rc=%d), retrying in 2 s\n", mqtt.state());
      delay(2000);
    }
  }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
// Read potentiometer and convert raw 0..4095 -> 0.0..3.3 V
float readPotVoltage() {
  int raw = analogRead(PIN_POT);
  return (raw * ADC_VREF) / ADC_MAX;
}

// Publish a float as a short text payload, like the Python publishers do.
void publishFloat(const char* topic, float value, uint8_t decimals = 2) {
  char buf[16];
  dtostrf(value, 0, decimals, buf);  // float -> "24.30"
  mqtt.publish(topic, buf);
  Serial.printf("Published %s -> %s\n", topic, buf);
}

// ---------------------------------------------------------------------------
// Setup / loop
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nESP32 MQTT publisher starting");

  dht.begin();
  analogReadResolution(12);  // 0..4095 (default on ESP32, set explicitly for clarity)

  connectWiFi();
  connectMQTT();
}

void loop() {
  // Keep MQTT connection alive and process incoming traffic.
  if (!mqtt.connected()) {
    connectMQTT();
  }
  mqtt.loop();

  // Time-based publishing (non-blocking - delay() would freeze mqtt.loop()).
  uint32_t now = millis();
  if (now - lastPublishMs < PUBLISH_INTERVAL_MS) {
    return;
  }
  lastPublishMs = now;

  // ---- DHT11 -------------------------------------------------------------
  float tempC    = dht.readTemperature();   // Celsius
  float humidity = dht.readHumidity();      // %RH

  if (isnan(tempC) || isnan(humidity)) {
    Serial.println("DHT11 read failed - skipping this cycle");
  } else {
    publishFloat("TEMPERATURE", tempC,    2);
    publishFloat("HUMIDITY",    humidity, 2);
  }

  // ---- Potentiometer -----------------------------------------------------
  float voltage = readPotVoltage();
  publishFloat("VOLTAGE", voltage, 3);
}
