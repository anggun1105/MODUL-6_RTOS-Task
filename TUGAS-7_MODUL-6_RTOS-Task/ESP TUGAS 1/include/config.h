#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================
 * Konfigurasi Pin dan Parameter - ESP32_01_Task_Create_Basic
 * ESP32 DOIT DevKit V1
 * ============================================================ */

// LED GPIOs
#define LED1_GPIO           2       // Built-in LED (onboard biru)
#define LED2_GPIO           4       // External LED

// Button GPIO
#define BUTTON_GPIO         0       // Tombol BOOT

// UART
#define UART_BAUD           115200

// Task parameters
#define TASK_STACK_SIZE     4096
#define TASK1_PRIORITY      3       // Prioritas tinggi untuk LED1
#define TASK2_PRIORITY      2       // Prioritas rendah untuk LED2
#define MONITOR_PRIORITY    1       // Prioritas terendah untuk monitor

// Timing (ms)
#define LED1_BLINK_MS       500     // LED1 blink setiap 500ms
#define LED2_BLINK_MS       200     // LED2 blink setiap 200ms
#define MONITOR_PERIOD_MS   2000    // Monitor report setiap 2 detik

// Data print interval
#define DATA_PRINT_INTERVAL 5       // Print DATA setiap N cycle

#endif // CONFIG_H
