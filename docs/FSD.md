**UTS IOT - Functional Specification Document**

**Nicholas Laurensius Halim**
**2802428072**

**1. Overview**

**1.1 Purpose**

This project is an IoT system that performs two main functions simultaneously. First, the main microcontroller reads environmental sensor data (light, temperature, humidity) and publishes it to an MQTT broker. Second, the system utilizes a mobile smartphone acting as an Edge AI device to distinguish between a blue pen and a black pen using a locally deployed machine learning model via a WebAssembly interface.

The reliability of the microcontroller operations is ensured by FreeRTOS, which handles multiple tasks independently. By utilizing FreeRTOS tasks and queues, each subsystem should operate asynchronously and efficiently without interfering with one another.

**1.2 System Context**

**A. Environmental Monitoring Flow (Sensors to MQTT Broker)**

- BH1750 reads light intensity. DHT20 reads temperature and humidity. Both use I2C so they share the same 2 wires.

- Sensor data gets pushed into a FreeRTOS Queue so the reading task and publishing task don\'t conflict.

- The publishing task pulls from the Queue and sends data to broker.emqx.io every 10 seconds.

- The system also listens for an override command via MQTT. When received, a Semaphore signals the LED task to switch to ON for 10 seconds.

**B. Edge AI Sorting Flow**

- A trained ML model is deployed and run on a mobile phone using the Edge Impulse mobile client.

- The phone camera takes a picture of the pen in a non-contact, non-invasive way.

- The result (Blue or Black) is shown directly on the phone screen as the sorting signal.

**2. Hardware Architecture & Pin Mapping**

**2.1 Component Used**

| **Main Components**  | **Specifications / Type** | **Functional Role in the System**                                             |
|----------------------|---------------------------|-------------------------------------------------------------------------------|
| Microcontroller Unit | ESP32D-CP2101             | Runs FreeRTOS tasks, handles WiFi, reads sensors, publishes to MQTT           |
| Edge AI Unit         | Smartphone / Mobile Phone | Captures live images of pens and runs the quantized ML sorting model locally. |
| Light Sensor         | BH1750                    | Measures how bright the room is (Lux)                                         |
| Enviroment Sensor    | DHT20                     | Measures temperature (C) and humidity (%)                                     |
| Output Indicator     | 5mm LED + 220 Ω Resistor  | Visual indicator for blink loop and override command                          |

**2.2 Pin Assignments**

**ESP32D-CP2101 GPIO (Main MCU)**

| **Code Constant** | **GPIO** | **I/O Type** | **Description & Hardware Limitations**               |
|-------------------|----------|--------------|------------------------------------------------------|
| PIN_LED_IND       | 2        | Output       | Controls the indicator LED. HIGH = ON.               |
| PIN_SDA           | 21       | I/O          | I2C data line. BH1750 and DHT20 both connected here. |
| PIN_SCL           | 22       | I/O          | I2C clock line. Same bus as SDA.                     |

**3. Functional Requirements**

**3.1 FreeRTOS Task Management**

- FRT-001: The system must use FreeRTOS Tasks to keep sensor reading, MQTT publishing, and LED control running separately. None of them should block each other.

- FRT-002: A FreeRTOS Queue is used to pass sensor readings from the reading task to the publishing task. This way the two tasks don\'t try to access the same variable at the same time.

- FRT-003: A FreeRTOS Semaphore or Event Group is used to notify the LED task when an MQTT override command is received. Using this instead of a simple flag variable makes sure the signal isn\'t missed even if the task is busy.

**3.2 MQTT Communication & Formatting**

- MQT-001: The system connects to xe961c31.ala.asia-southeast1.emqxsl.com with a username and password. This stops random people from subscribing to our sensor topics or sending fake override commands.

- MQT-002: Sensor data is published every 10 seconds. The timing is maintained by the FreeRTOS task using vTaskDelayUntil so it stays accurate.

- MQT-003: Based on NIM 2802428072, the delay values are X=7 and Y=2. Each topic has its own permanent publish schedule that is offset from each other:
- LightN : t=0s, 10s, 20s, 30s, \... (every 10 seconds starting at boot)
- TemperatureL : t=7s, 17s, 27s, 37s, \... (every 10 seconds starting at X=7s)
- HumidityH : t=9s, 19s, 29s, 39s, \... (every 10 seconds starting at X+Y=9s)
- The offset is permanent, not just the first cycle. Each topic always stays offset from the others by X and Y seconds.

- MQT-004: Name is Nicholas Laurensius Halim (3 words), so initials are N, L, H. Topics are:
- LightN
- TemperatureL
- HumidityH

**3.3 LED Control**

- LED-001: By default, the LED blinks: 1 second ON, 1 second OFF, in a continuous loop. This runs in its own FreeRTOS task.

- LED-002: When the MQTT override command is received, the LED immediately switches to and stays ON for exactly 10 seconds. After that it goes back to the 1s/1s blink loop. The Semaphore makes sure this override signal is not missed even if the LED task is in the middle of a delay.

**3.4 Edge AI Sorting**

- EAI-001: The system identifies pens using a phone camera. The phone does not touch the pen. This is the non-contact, non-invasive method required.

- EAI-002: The trained model must sort with at least 90% accuracy between blue and black pens. The model was trained on Edge Impulse with 150 photos per class.

- EAI-003: The sorting result is shown on the mobile phone screen through the Edge Impulse mobile inferencing client. Output shows the detected class (Blue or Black) and its confidence score. This tells the operator which bin to direct the pen to.

**4. Network & Data Flow**

**4.1 Communication Protocol**

| **Data Type/ Component** | **Communication Protocol** | **Port/ Security** | **Function Description**                                       |
|--------------------------|----------------------------|--------------------|----------------------------------------------------------------|
| Sensor Data Telemetry    | MQTT                       | Port 8883 / Auth   | ESP32D publishes light, temp, humidity to broker.emqx.io       |
| Override Command         | MQTT                       | Port 8883 / Auth   | ESP32D subscribes to an override topic to receive LED commands |
| AI Sorting Output        | Edge Impulse Mobile Client | On-device / None   | Phone camera runs inference locally, result shown on screen    |

**4.2 Data Flow**

1.  System Initialization (Edge): FreeRTOS starts and allocates Queue and Semaphore. ESP32D connects to WiFi. Then it connects to EMQX broker with credentials and subscribes to the override topic.

2.  Sensor Polling (Task 1): Every polling interval, the ESP32 reads BH1750 for lux and DHT20 for temp/humidity via I2C. It packages the values and puts them into the Queue.

3.  Data Transmission (Task 2): This task pulls the latest sensor data from the shared variable. Each topic has its own permanent publish schedule based on NIM 2802428072 (X=7, Y=2): LightN publishes at t=0s then every 10s, TemperatureL at t=7s then every 10s, and HumidityH at t=9s then every 10s. This offset is permanent throughout the entire runtime, not just the first cycle.

4.  Default LED Loop (Task 3): LED blinks 1s ON / 1s OFF in a loop. This task waits for the Semaphore. When it gets the Semaphore, it skips the blink loop and turn the LED ON for 10 seconds then resumes blinking.

5.  Remote Override Trigger (Interrupt): When MQTT receives a payload on the override topic, the callback immediately gives the Semaphore. Task 3 receive and does the 10-second LED override.

6.  Edge AI Sorting (Mobile Phone): The operator opens the Edge Impulse mobile inferencing client on their phone. The phone camera captures the pen image and runs the quantized MobileNet model locally on-device. The result which is the confidence score appear on screen immediately.

**5. Expected Message Format**

**5.1 MQTT Publish Payloads (ESP32D -\> Broker)**

| **Topic**    | **Example Payload** | **Unit** | **Notes**               |
|--------------|---------------------|----------|-------------------------|
| LightN       | 432.50              | Lux      | Float, 2 decimal places |
| TemperatureL | 28.30               | Celsius  | Float, 2 decimal places |
| HumidityH    | 65.10               | %        | Float, 2 decimal places |

**5.2 MQTT Override Command (Client -\> ESP32D)**

The ESP32D subscribes to a specific override topic. The expected incoming payload to trigger the LED override is:

**Topic: \"LEDOverrideN\"**

Payload: \"ON\"

When the ESP32D receives this exact payload, it gives the Semaphore to Task_LEDControl. Any other payload on this topic is ignored. The comparison is case-sensitive.

**5.3 Edge AI Sorting Output (Mobile Phone)**

| **Classification Result**  | **Display Output**                                        |
|----------------------------|-----------------------------------------------------------|
| Blue pen detected          | BLUE - confidence \> 90%                                  |
| Black pen detected         | BLACK - confidence \> 90%                                 |
| Below confidence threshold | Result shown with low confidence score, operator re-scans |

Inference runs fully on-device on the phone; no data is sent to a server. The model used is a quantized (int8) MobileNet V2 exported from Edge Impulse as an Android library.

**6. Non-Functional Requirements**

**6.1 Performance**

- PRF-001: The 10-second MQTT publish interval should not be affected by the LED blink task.

- PRF-002: The Edge AI inference on the mobile phone should finish within 2 seconds per item so the sorting line does not have to wait long for a result.

**6.2 Reliability**

- REL-001: If the WiFi drops or the broker disconnects, ESP32 should try to reconnect automatically without freezing or crashing the sensor reading tasks.

- REL-002: The queue depth is set so that the sensor readings don\'t drop right away if the publish task is slow.

**7. Software & Libraries**

**7.1 Framework & Platform**

| **Item**     | **Details** |
|--------------|-------------|
| Framework    | Arduino     |
| Build System | PlatformIO  |
| Target Board | esp32dev    |
| Platform     | espressif32 |

**7.2 Library Dependecies**

| **Library**                       | **Source**                       | **Purpose**                                                              |
|-----------------------------------|----------------------------------|--------------------------------------------------------------------------|
| PubSubClient @ \^2.8              | knolleary/PubSubClient           | MQTT client for publishing sensor data and subscribing to override topic |
| DHT20                             | robtillaart/DHT20                | Read temperature and humidity from DHT20 sensor via I2C                  |
| BH1750 @ \^1.3.0                  | claws/BH1750                     | Read ambient light intensity (Lux) from BH1750 sensor via I2C            |
| Adafruit BusIO @ \^1.14.0         | adafruit/Adafruit BusIO          | I2C/SPI abstraction layer, dependency of BH1750                          |
| Adafruit Unified Sensor @ \^1.1.9 | adafruit/Adafruit Unified Sensor | Common sensor interface, dependency of BH1750                            |

**8. User Interface**

**8.1 MQTT Dashboard**

Monitoring is done using MQTT Explorer or Node-RED. The dashboard shows:

- Live graph of LightN (Lux) update every 10 seconds

- Live reading of TemperatureL (Celsius)

- Live reading of HumidityH (%)

**8.2 Sending Override Command**

To trigger the LED override, the user publishes a payload to the subscribed override topic using the MQTT Client. The LED on the ESP32 will immediately turn ON and stay ON for 10 seconds. Only authenticated users (with correct credentials) can do this.
