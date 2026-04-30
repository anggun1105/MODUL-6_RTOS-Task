/* ============================================================
 * ESP32 FreeRTOS Task dengan LED dan DHT22 (UPDATED GPIO)
 * ============================================================ */

#include <Arduino.h>
#include <DHT.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* Tag untuk logging */
#define TAG "ESP32_TASKS"

/* Definisi GPIO (SUDAH DIUBAH) */
#define LED_GPIO    4
#define DHT_GPIO    5

/* DHT object */
DHT dht(DHT_GPIO, DHT22);

/* Task handles */
static TaskHandle_t xLedTaskHandle = NULL;
static TaskHandle_t xDhtTaskHandle = NULL;

/* Counter untuk tracking */
static volatile uint32_t led_counter = 0;
static volatile uint32_t dht_counter = 0;

/* ============================================================
 * TASK LED
 * ============================================================ */
void led_task(void *pvParameters) {
    pinMode(LED_GPIO, OUTPUT);

    bool led_state = false;

    while (1) {
        led_state = !led_state;
        digitalWrite(LED_GPIO, led_state);

        uint32_t timestamp = millis();
        int core_id = xPortGetCoreID();
        led_counter++;

        Serial.printf("LED Task  | %7lums | State=%s | Count=%4lu | Core=%d\n",
                      timestamp,
                      led_state ? "ON" : "OFF",
                      led_counter,
                      core_id);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ============================================================
 * TASK DHT22
 * ============================================================ */
void dht_task(void *pvParameters) {
    while (1) {
        float temperature = dht.readTemperature();
        float humidity = dht.readHumidity();

        dht_counter++;

        if (!isnan(temperature) && !isnan(humidity)) {
            Serial.printf("DHT22 Task | %7lums | Temp=%5.1fC | Hum=%5.1f%% | Count=%4lu\n",
                          millis(), temperature, humidity, dht_counter);
        } else {
            Serial.printf("DHT22 Task | %7lums | Temp=ERROR | Hum=---- | Count=%4lu\n",
                          millis(), dht_counter);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ============================================================
 * SETUP
 * ============================================================ */
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("==========================================");
    Serial.println("ESP32 FreeRTOS Tasks Demo (GPIO UPDATED)");
    Serial.println("LED = GPIO 4 | DHT22 = GPIO 5");
    Serial.println("==========================================");
    Serial.println();

    dht.begin();

    xTaskCreate(led_task, "LED_Task", 2048, NULL, 1, &xLedTaskHandle);
    xTaskCreate(dht_task, "DHT_Task", 4096, NULL, 1, &xDhtTaskHandle);

    Serial.println("▶️  Tasks started");
}

/* ============================================================ */
void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}