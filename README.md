# ESP32 MQTT Sensor Demo

A two-part project: an **ESP32 firmware** that reads sensor data and publishes it over MQTT, and a **Python subscriber** that receives and prints those readings in real time.

---

## How it works

```
DHT11 (temp/humidity)  ──┐
                          ├──► ESP32 ──► broker.hivemq.com ──► mqtt_subscribe.py
Potentiometer (voltage) ──┘
```

The ESP32 connects to Wi-Fi, then connects to a public HiveMQ MQTT broker and publishes three topics every second. The Python script subscribes to those same topics and prints each incoming message.

---

## Project structure

```
esp32_example/
├── esp32_mqtt_publisher/
│   ├── esp32_mqtt_publisher.ino   # Arduino/ESP32 firmware
│   └── platformio.ini             # PlatformIO build config
└── mqtt_subscribe.py              # Python MQTT subscriber
```

---

## MQTT topics

| Topic         | Payload example | Source                        |
|---------------|-----------------|-------------------------------|
| `TEMPERATURE` | `24.30`         | DHT11 sensor on GPIO 4        |
| `HUMIDITY`    | `58.00`         | DHT11 sensor on GPIO 4        |
| `VOLTAGE`     | `1.652`         | Potentiometer wiper on GPIO 34|

All payloads are plain UTF-8 strings representing floating-point numbers.

---

## Hardware

### Wiring

| Component    | Pin        | ESP32 pin |
|--------------|------------|-----------|
| DHT11        | VCC        | 3V3       |
| DHT11        | Data       | GPIO 4    |
| DHT11        | GND        | GND       |
| Potentiometer| Left leg   | 3V3       |
| Potentiometer| Wiper      | GPIO 34   |
| Potentiometer| Right leg  | GND       |

GPIO 34 is input-only and ADC-capable (ADC1_CH6), making it suitable for analog reads.

### ADC conversion

The ESP32 ADC is configured at 12-bit resolution, so raw readings range from 0 to 4095, mapped linearly to 0–3.3 V:

```
voltage = (raw / 4095) * 3.3
```

---

## Firmware (`esp32_mqtt_publisher.ino`)

### Key settings (edit before flashing)

| Constant         | Default value              | Purpose                        |
|------------------|----------------------------|--------------------------------|
| `WIFI_SSID`      | `"Viwe S24 Ultra"`         | Your Wi-Fi network name        |
| `WIFI_PASSWORD`  | `"xhive7wifi"`             | Your Wi-Fi password            |
| `MQTT_BROKER`    | `"broker.hivemq.com"`      | MQTT broker address            |
| `MQTT_PORT`      | `1883`                     | Standard unencrypted MQTT port |
| `MQTT_CLIENT_ID` | `"ESP32_SmartFarm_Node01"` | Must be unique on the broker   |
| `PUBLISH_INTERVAL_MS` | `1000`              | Milliseconds between publishes |

### Firmware flow

1. **`setup()`** — initialises Serial (115200 baud), starts the DHT11, sets ADC resolution to 12-bit, connects to Wi-Fi, then connects to the MQTT broker.
2. **`loop()`** — calls `mqtt.loop()` every iteration to keep the connection alive. Every `PUBLISH_INTERVAL_MS` milliseconds it reads the DHT11 and potentiometer and publishes three topics.
3. **`connectWiFi()`** — blocks until `WiFi.status() == WL_CONNECTED`, printing dots to Serial while waiting.
4. **`connectMQTT()`** — retries the broker connection every 2 seconds until it succeeds.
5. **`readPotVoltage()`** — reads the ADC and scales the result to volts.
6. **`publishFloat()`** — converts a float to a string with `dtostrf()` and publishes it.

If the DHT11 returns `NaN` (sensor not ready or wiring fault), that cycle is skipped and a warning is printed to Serial.

### Libraries required

Install via Arduino Library Manager **or** let PlatformIO handle it automatically:

- **PubSubClient** by Nick O'Leary (`knolleary/PubSubClient @ ^2.8`)
- **DHT sensor library** by Adafruit (`adafruit/DHT sensor library @ ^1.4.6`)
- **Adafruit Unified Sensor** (dependency of DHT) (`adafruit/Adafruit Unified Sensor @ ^1.1.14`)

### Building with PlatformIO

```bash
pio run                   # compile
pio run --target upload   # compile and flash
pio device monitor        # open Serial monitor at 115200 baud
```

### Building with Arduino IDE

1. Install the three libraries above via **Sketch → Include Library → Manage Libraries**.
2. Select **Tools → Board → ESP32 Arduino → ESP32 Dev Module**.
3. Click **Upload**.

---

## Python subscriber (`mqtt_subscribe.py`)

### Requirements

```bash
pip install paho-mqtt
```

### Running

```bash
python mqtt_subscribe.py
```

The script connects to `broker.hivemq.com:1883` with client ID `Smartphone`, subscribes to `TEMPERATURE`, `HUMIDITY`, and `VOLTAGE`, and prints every incoming message for 60 seconds before exiting cleanly.

### Changing the run duration

Edit the `time.sleep(60)` line to however many seconds you want the subscriber to stay active. For continuous operation, replace it with a `while True: time.sleep(1)` loop (remember to keep `Ctrl+C` handling in place).

### Error handling

| Exception            | Cause                                      |
|----------------------|--------------------------------------------|
| `ConnectionRefusedError` | Wrong broker address or port blocked   |
| `KeyboardInterrupt`  | User pressed Ctrl+C; disconnects cleanly   |
| `Exception`          | Any other error; message printed to console|

---

## Security note

The credentials in `esp32_mqtt_publisher.ino` (`WIFI_PASSWORD`) are stored in plain text. This is fine for local development and demos, but before sharing or committing the code, move secrets to a separate `secrets.h` file and add it to `.gitignore`.

The HiveMQ public broker is unauthenticated and unencrypted — do not publish sensitive data to it.
