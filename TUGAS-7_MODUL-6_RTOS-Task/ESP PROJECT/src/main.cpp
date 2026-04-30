/* ============================================================
 * ESP32 FreeRTOS Task dengan LED, DHT22, dan Thermocouple
 * ============================================================ */

#include <Arduino.h>
#include <DHT.h>
#include <max6675.h> 
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* Tag untuk logging */
#define TAG "ESP32_TASKS"

/* Definisi GPIO */
#define LED_GPIO    4
#define DHT_GPIO    5

/* Definisi GPIO Thermocouple MAX6675
 * Karena D18 diminta, kita gunakan D18 (GPIO 18) sebagai CS.
 * Pin DO (MISO) dan CLK (SCK) menggunakan pin default standar ESP32.
 */
#define THERMO_CS   18  // Chip Select di D18
#define THERMO_DO   19  // Data Out (MISO)
#define THERMO_CLK  21  // Clock (SCK)

/* Object Sensor */
DHT dht(DHT_GPIO, DHT22);
MAX6675 thermocouple(THERMO_CLK, THERMO_CS, THERMO_DO);

/* Task handles */
static TaskHandle_t xLedTaskHandle = NULL;
static TaskHandle_t xDhtTaskHandle = NULL;
static TaskHandle_t xThermoTaskHandle = NULL; // Handle Task 3

/* Counter untuk tracking */
static volatile uint32_t led_counter = 0;
static volatile uint32_t dht_counter = 0;
static volatile uint32_t thermo_counter = 0; // Counter Task 3

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

        Serial.printf("LED Task   | %7lums | State=%s | Count=%4lu | Core=%d\n",
                      timestamp,
                      led_state ? "ON " : "OFF",
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
 * TASK THERMOCOUPLE (TASK 3)
 * ============================================================ */
void thermo_task(void *pvParameters) {
    while (1) {
        // Modul MAX6675 membutuhkan waktu sekitar 250ms antar pembacaan
        float thermo_temp = thermocouple.readCelsius();
        thermo_counter++;

        // isnan() digunakan untuk mengecek apakah pembacaan valid
        if (!isnan(thermo_temp)) {
            Serial.printf("Thermo Task| %7lums | Temp=%5.2fC |          | Count=%4lu\n",
                          millis(), thermo_temp, thermo_counter);
        } else {
            Serial.printf("Thermo Task| %7lums | Temp=ERROR |          | Count=%4lu\n",
                          millis(), thermo_counter);
        }

        vTaskDelay(pdMS_TO_TICKS(1500)); // Delay sampling 1.5 detik
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
    Serial.println("LED = 4 | DHT22 = 5 | Thermo CS = 18");
    Serial.println("==========================================");
    Serial.println();

    dht.begin();

    // Pembuatan Tasks
    xTaskCreate(led_task, "LED_Task", 2048, NULL, 1, &xLedTaskHandle);
    xTaskCreate(dht_task, "DHT_Task", 4096, NULL, 1, &xDhtTaskHandle);
    xTaskCreate(thermo_task, "Thermo_Task", 2048, NULL, 1, &xThermoTaskHandle);

    Serial.println("▶️  Tasks started");
}

/* ============================================================ */
void loop() {
    // Loop kosong karena scheduler FreeRTOS yang menangani eksekusi
    vTaskDelay(pdMS_TO_TICKS(1000));
}