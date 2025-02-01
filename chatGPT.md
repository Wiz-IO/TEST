Below is an example QuecPython program that combines GPS‐position acquisition, MQTT telemetry and an LED indicator on a Quectel EC200E‑EU module. In this example, the built‑in GNSS (GPS) is started and queried periodically; its latitude and longitude are published via MQTT (using, for example, the public HiveMQ broker), and an LED is toggled briefly each time data is sent. (Make sure to adjust the pin numbers, network configuration, MQTT broker settings and any GNSS API calls to match your actual hardware and firmware version.)

> **Note:**  
> The exact APIs available on your firmware may differ. For example, Quectel’s QuecPython firmware may provide a module such as `ql_gps` (or similar) for accessing GNSS data. In addition, the MQTT library (here using a MicroPython‑style client from `umqtt.simple`) and the LED’s GPIO pin number might need adjustment. Use this example as a starting point and consult your module’s documentation for details.

```python
"""
Example GPS Tracker with MQTT Telemetry and LED Indicator
for Quectel EC200E-EU running QuecPython.
"""

import machine
import time
import ubinascii

# --- Configuration Section ---

# LED configuration: adjust the pin number as appropriate.
# For example, here we assume the onboard LED is on GPIO2.
LED_PIN = 2

# MQTT Broker settings
MQTT_BROKER   = "broker.hivemq.com"  # Change as needed.
MQTT_PORT     = 1883
MQTT_TOPIC    = "gps/tracker"
CLIENT_ID     = ubinascii.hexlify(machine.unique_id())  # Unique client ID

# GPS polling interval in seconds.
GPS_POLL_INTERVAL = 30

# --- End of Configuration ---


# Initialize LED (configured as output)
led = machine.Pin(LED_PIN, machine.Pin.OUT)

# --- (Optional) Initialize Cellular or Wi-Fi network connection ---
# For a cellular device like the EC200E, the network may already be active
# or require initialization via AT commands or a provided API.
# For example:
# import network
# cellular = network.Cellular()
# cellular.attach(apn="your_apn_here")
# while not cellular.isconnected():
#     time.sleep(1)
# print("Network connected:", cellular.ifconfig())


# --- Initialize MQTT Client ---
try:
    from umqtt.simple import MQTTClient
except ImportError:
    raise ImportError("MQTT library not found. Please ensure umqtt.simple is available.")

mqtt_client = MQTTClient(CLIENT_ID, MQTT_BROKER, port=MQTT_PORT)
try:
    mqtt_client.connect()
    print("Connected to MQTT broker:", MQTT_BROKER)
except Exception as e:
    print("Failed to connect to MQTT broker:", e)
    raise


# --- Initialize and Start the GPS ---
# This example assumes that the module provides a 'ql_gps' (or similar) module.
# Please refer to your Quectel documentation for the correct API calls.
try:
    import ql_gps
except ImportError:
    raise ImportError("GPS module not found. Please check your firmware documentation.")

def init_gps():
    """
    Initializes the GPS (GNSS) engine.
    Adjust or add AT command sequences or API calls as required.
    """
    # Example: start the GPS service.
    ret = ql_gps.start()  # This function name may differ.
    if ret != 0:
        print("Failed to start GPS, error code:", ret)
    else:
        print("GPS started successfully.")

def get_gps_position():
    """
    Retrieves the current GPS position.
    Returns a dictionary with latitude and longitude if available.
    """
    # The API below is an example. Replace with your module’s API.
    pos = ql_gps.get_position()  # Expected to return a dict, e.g., {"lat": ..., "lon": ...}
    if pos is None:
        print("GPS data not available yet.")
        return None
    return pos

# Start the GPS engine.
init_gps()


# --- Main Loop: Acquire GPS data, send via MQTT, and toggle LED ---
while True:
    gps_data = get_gps_position()
    if gps_data:
        # Format the payload. Adjust keys according to the actual API.
        payload = "lat: {}, lon: {}".format(gps_data.get("lat", "NA"), gps_data.get("lon", "NA"))
        print("Publishing payload:", payload)
        
        try:
            # Turn on LED to indicate transmission
            led.value(1)
            mqtt_client.publish(MQTT_TOPIC, payload)
            print("Data published to topic:", MQTT_TOPIC)
        except Exception as e:
            print("Failed to publish MQTT message:", e)
        finally:
            # Turn LED off after sending
            led.value(0)
    
    # Wait for the next polling interval
    time.sleep(GPS_POLL_INTERVAL)
```

---

### Explanation

1. **Network and MQTT Setup:**  
   In many Quectel modules the network (cellular or Wi‑Fi) is already configured or requires initialization via provided APIs. Adjust the code as necessary. The MQTT client is set up using a MicroPython‑compatible library (`umqtt.simple`).

2. **GPS Initialization and Data Retrieval:**  
   The functions `init_gps()` and `get_gps_position()` use a hypothetical `ql_gps` module. Replace these calls with the actual API calls for your firmware if they differ.

3. **LED Indication:**  
   An LED connected to a specified GPIO pin is turned on while sending the data and then turned off. Adjust the pin number based on your hardware’s wiring.

4. **Main Loop:**  
   The loop retrieves the GPS data, constructs a telemetry payload, publishes it via MQTT, and then waits for the next interval.

---

Be sure to test and adjust the code in your development environment. This example is intended as a starting point for integrating GPS, MQTT telemetry, and LED status indication on a Quectel EC200E‑EU module using QuecPython.
