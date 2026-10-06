# IoT Edge Architecture: FreeRTOS MQTT Sensor Hub & TinyML Classifier

A collection of embedded IoT applications demonstrating real-time multitasking with FreeRTOS, authenticated MQTT telemetry, and Edge AI (TinyML) deployment on resource-constrained microcontrollers.

---

## Part 1: FreeRTOS Multi-Tasking & Authenticated MQTT Telemetry

An embedded telemetry system developed on the ESP32 to concurrently acquire environmental metrics and communicate with a cloud MQTT broker without blocking CPU execution.

### Key Technical Implementations
* **Real-Time Task Scheduling:** Orchestrated concurrent sensor sampling using FreeRTOS Tasks and inter-task communication via FreeRTOS Queues.
* **Sensor Interfacing:** Interfaced with BH1750 (Ambient Light Sensor) and DHT10/DHT20 (Temperature & Humidity) via I2C/Digital buses.
* **Telemetry & Synchronization:** Published formatted sensor telemetry every 10 seconds to distinct authenticated MQTT topics. Managed LED state toggling and priority overrides (10-second hold) using FreeRTOS synchronization primitives (Semaphores / Event Groups).
* **MQTT Security:** Configured credentials-based authentication to secure communication channels against unauthorized broker access.

> **Documentation & Code:** Implementation details and the Functional Specification Document (FSD) are located in the [`/MQTT_FreeRTOS_SensorHub`](./MQTT_FreeRTOS_SensorHub) directory.

---

## Part 2: TinyML Vision Classifier on the Edge (Edge Impulse)

An embedded Computer Vision inference pipeline deployed directly onto a microcontroller to classify physical objects (Black Pen vs. Blue Pen) in real time.

### Key Technical Implementations
* **Edge Intelligence (TinyML):** Trained and quantized a lightweight image classification neural network using Edge Impulse, optimized for minimal RAM and flash footprints on microcontrollers.
* **Inference & Sorting:** Performed non-contact inference achieving >90% validation accuracy, transmitting deterministic classification results over UART Serial.
* **Ultra-Low Latency:** Eliminated cloud transmission overhead by executing inference purely on-device at the edge.

> **Model & Code:** Exported C++ model libraries and serial execution firmware are located in the [`/TinyML_Pen_Classifier`](./TinyML_Pen_Classifier) directory.

---

## Demonstration & Defense
* **MQTT & FreeRTOS Architecture Walkthrough:** https://drive.google.com/file/d/1MenlbeiK6Oy9RnpoRL_Zk1ZJMAEkeGnU/view?usp=sharing
* **TinyML Edge Classification Demo:** https://drive.google.com/file/d/1mQfihKbS8RQlDPVFaOqhLTKA-Y6BQHzu/view?usp=sharing
