#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <BH1750.h>
#include <DHT20.h>
#include "config.h"

BH1750 lightMeter;
DHT20 dht;                  

WiFiClientSecure espClient;
PubSubClient     mqttClient(espClient);


typedef struct {
    float lux;
    float temperature;
    float humidity;
} SensorData_t;

QueueHandle_t   sensorQueue;        
SemaphoreHandle_t overrideSemaphore; 

void connectWiFi();
void connectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);

void task_SensorRead   (void* pvParameters);
void task_MQTTPublish  (void* pvParameters);
void task_LEDControl   (void* pvParameters);

void connectWiFi() {
    Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }
    Serial.printf("\n[WiFi] Connected. IP: %s\n", WiFi.localIP().toString().c_str());
}

void connectMQTT() {
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    while (!mqttClient.connected()) {
        Serial.printf("[MQTT] Connecting to %s ...\n", MQTT_BROKER);
        if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
            Serial.println("[MQTT] Connected.");
            mqttClient.subscribe(TOPIC_LED_OVERRIDE);
            Serial.printf("[MQTT] Subscribed to: %s\n", TOPIC_LED_OVERRIDE);
        } else {
            Serial.printf("[MQTT] Failed, rc=%d. Retry in 3s...\n", mqttClient.state());
            vTaskDelay(pdMS_TO_TICKS(3000));
        }
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    char msg[length + 1];
    memcpy(msg, payload, length);
    msg[length] = '\0';

    Serial.printf("[MQTT] Message on [%s]: %s\n", topic, msg);

    if (strcmp(topic, TOPIC_LED_OVERRIDE) == 0 && strcmp(msg, "ON") == 0) {
        Serial.println("[MQTT] Override command received → giving Semaphore");
        xSemaphoreGiveFromISR(overrideSemaphore, NULL);
    }
}


static SensorData_t latestData = {0, 0, 0};
static SemaphoreHandle_t dataMutex;

void task_SensorRead(void* pvParameters) {
    Serial.println("[Task1] SensorRead started.");

    while (true) {
        SensorData_t data;

        data.lux = lightMeter.readLightLevel();
        if (data.lux < 0) {
            Serial.println("[Task1] BH1750 read failed, using 0.");
            data.lux = 0.0f;
        }

        dht.read();
        data.temperature = dht.getTemperature();
        data.humidity    = dht.getHumidity();

        if (isnan(data.temperature) || isnan(data.humidity)) {
            Serial.println("[Task1] DHT20 read failed, using 0.");
            data.temperature = 0.0f;
            data.humidity    = 0.0f;
        }

        Serial.printf("[Task1] Read → Lux: %.2f | Temp: %.2f°C | Hum: %.2f%%\n",
                      data.lux, data.temperature, data.humidity);

        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            latestData = data;
            xSemaphoreGive(dataMutex);
        }

        xQueueSend(sensorQueue, &data, pdMS_TO_TICKS(100));

        vTaskDelay(pdMS_TO_TICKS(1000));  
    }
}

void task_MQTTPublish(void* pvParameters) {
    Serial.println("[Task2] MQTTPublish started.");

    char payload[16];

    TickType_t startTick = xTaskGetTickCount();
    TickType_t nextLight    = startTick + pdMS_TO_TICKS(0);
    TickType_t nextTemp     = startTick + pdMS_TO_TICKS(DELAY_X_SEC * 1000UL);
    TickType_t nextHumidity = startTick + pdMS_TO_TICKS((DELAY_X_SEC + DELAY_Y_SEC) * 1000UL);
    const TickType_t interval = pdMS_TO_TICKS(PUBLISH_INTERVAL_MS);

    while (true) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[Task2] WiFi lost, reconnecting...");
            connectWiFi();
        }
        if (!mqttClient.connected()) {
            Serial.println("[Task2] MQTT lost, reconnecting...");
            connectMQTT();
        }
        mqttClient.loop();

        TickType_t now = xTaskGetTickCount();

        if ((TickType_t)(now - nextLight) < (TickType_t)(portMAX_DELAY / 2)) {
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                snprintf(payload, sizeof(payload), "%.2f", latestData.lux);
                xSemaphoreGive(dataMutex);
            }
            mqttClient.publish(TOPIC_LIGHT, payload);
            Serial.printf("[Task2] t=%lums | Published %s → %s\n",
                          (unsigned long)pdTICKS_TO_MS(now), TOPIC_LIGHT, payload);
            nextLight += interval;
        }

        if ((TickType_t)(now - nextTemp) < (TickType_t)(portMAX_DELAY / 2)) {
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                snprintf(payload, sizeof(payload), "%.2f", latestData.temperature);
                xSemaphoreGive(dataMutex);
            }
            mqttClient.publish(TOPIC_TEMPERATURE, payload);
            Serial.printf("[Task2] t=%lums | Published %s → %s\n",
                          (unsigned long)pdTICKS_TO_MS(now), TOPIC_TEMPERATURE, payload);
            nextTemp += interval;
        }

        if ((TickType_t)(now - nextHumidity) < (TickType_t)(portMAX_DELAY / 2)) {
            if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                snprintf(payload, sizeof(payload), "%.2f", latestData.humidity);
                xSemaphoreGive(dataMutex);
            }
            mqttClient.publish(TOPIC_HUMIDITY, payload);
            Serial.printf("[Task2] t=%lums | Published %s → %s\n",
                          (unsigned long)pdTICKS_TO_MS(now), TOPIC_HUMIDITY, payload);
            nextHumidity += interval;
        }

        mqttClient.loop();
        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}

void task_LEDControl(void* pvParameters) {
    Serial.println("[Task3] LEDControl started.");
    pinMode(PIN_LED_IND, OUTPUT);
    digitalWrite(PIN_LED_IND, LOW);

    bool ledState = false;

    while (true) {
        if (xSemaphoreTake(overrideSemaphore, 0) == pdTRUE) {
            Serial.println("[Task3] Override! LED ON for 10 seconds.");
            digitalWrite(PIN_LED_IND, HIGH);
            vTaskDelay(pdMS_TO_TICKS(LED_OVERRIDE_MS));
            digitalWrite(PIN_LED_IND, LOW);
            ledState = false;
            Serial.println("[Task3] Override done. Resuming blink loop.");
        } else {
            ledState = !ledState;
            digitalWrite(PIN_LED_IND, ledState ? HIGH : LOW);
            vTaskDelay(pdMS_TO_TICKS(LED_BLINK_MS));
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== UTS IOT Nicholas Booting ===");

    Wire.begin(PIN_SDA, PIN_SCL);

    if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
        Serial.println("[Setup] ERROR: BH1750 not found! Check wiring.");
    } else {
        Serial.println("[Setup] BH1750 OK.");
    }

    dht.begin();
    Serial.println("[Setup] DHT20 OK.");

    espClient.setInsecure();

    connectWiFi();

    connectMQTT();

    sensorQueue = xQueueCreate(5, sizeof(SensorData_t));
    if (sensorQueue == NULL) {
        Serial.println("[Setup] FATAL: Failed to create sensorQueue!");
        while (true);
    }

    dataMutex = xSemaphoreCreateMutex();
    if (dataMutex == NULL) {
        Serial.println("[Setup] FATAL: Failed to create dataMutex!");
        while (true);
    }

    overrideSemaphore = xSemaphoreCreateBinary();
    if (overrideSemaphore == NULL) {
        Serial.println("[Setup] FATAL: Failed to create overrideSemaphore!");
        while (true);
    }

    xTaskCreatePinnedToCore(
        task_SensorRead,
        "SensorRead",
        4096,
        NULL,
        2,
        NULL,
        1
    );

    xTaskCreatePinnedToCore(
        task_MQTTPublish,
        "MQTTPublish",
        8192,
        NULL,
        2,
        NULL,
        0
    );

    xTaskCreatePinnedToCore(
        task_LEDControl,
        "LEDControl",
        2048,
        NULL,
        1,
        NULL,
        1
    );

    Serial.println("[Setup] All tasks created. System running.");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}