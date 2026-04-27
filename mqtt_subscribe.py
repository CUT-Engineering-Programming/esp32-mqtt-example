import paho.mqtt.client as mqtt  # MQTT client library for connecting to brokers
import time  # Standard library for sleep/timing

def on_message(client, userdata, message):                       # Callback fired whenever a subscribed message arrives
    print("Received message on topic '" + message.topic + "': "  # Print the topic the message came from
          + str(message.payload.decode("utf-8")))                # Decode the raw bytes payload to a readable string

mqttBroker = "broker.hivemq.com"  # Public HiveMQ broker address (no auth required)
client = None                     # Pre-declare client so the except blocks can reference it safely

try:
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, "Smartphone")  # Create client with ID "Smartphone" using the v2 callback API
    client.connect(mqttBroker, port=1883, keepalive=60)                   # Open TCP connection to broker; send a ping every 60 s to stay alive

    client.loop_start()                 # Start background thread that handles network I/O and dispatches callbacks
    client.subscribe("TEMPERATURE")     # Tell broker to forward messages published on TEMPERATURE
    client.subscribe("HUMIDITY")        # Tell broker to forward messages published on HUMIDITY
    client.subscribe("VOLTAGE")         # Tell broker to forward messages published on VOLTAGE
    client.on_message = on_message      # Register the callback that runs when any subscribed message arrives
    time.sleep(60)                      # Block the main thread for 60 seconds, keeping the subscriber alive
    client.loop_stop()                  # Stop the background network thread cleanly after the sleep

except ConnectionRefusedError:  # Broker actively rejected the connection (wrong address, port blocked, etc.)
    print("Connection refused. Check broker address and network.")
except KeyboardInterrupt:       # User pressed Ctrl+C to stop the script early
    print("Subscriber stopped.")
    if client is not None:      # Only clean up if the client was successfully created
        client.loop_stop()      # Stop the background thread
        client.disconnect()     # Send MQTT DISCONNECT packet so the broker knows we left cleanly
except Exception as e:          # Catch any other unexpected error and surface its message
    print(f"An error occurred: {e}")
