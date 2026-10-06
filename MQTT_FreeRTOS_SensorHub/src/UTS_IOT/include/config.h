#pragma once

#define WIFI_SSID       "Laura 1"
#define WIFI_PASSWORD   "cicikoko88"

#define MQTT_BROKER     "xe961c31.ala.asia-southeast1.emqxsl.com"
#define MQTT_PORT       8883
#define MQTT_USERNAME   "Nicholas"
#define MQTT_PASSWORD   "2802428072"
#define MQTT_CLIENT_ID  "ESP32_NLH_2802428072"

// Nicholas Laurensius Halim = N, L, H
#define TOPIC_LIGHT         "LightN"
#define TOPIC_TEMPERATURE   "TemperatureL"
#define TOPIC_HUMIDITY      "HumidityH"
#define TOPIC_LED_OVERRIDE  "LEDOverrideN"

#define PIN_LED_IND     2
#define PIN_SDA         21
#define PIN_SCL         22

// NIM 2802428072 = X=7, Y=2
#define DELAY_X_SEC         7       
#define DELAY_Y_SEC         2       
#define PUBLISH_INTERVAL_MS 10000   // 10 dtk
#define LED_OVERRIDE_MS     10000  
#define LED_BLINK_MS        1000    
