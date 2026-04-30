/**
 * ============================================================================
 * FILE: FreeRTOSConfig.h
 * PROJECT: FreeRTOS Multi-tasking Demo
 * 
 * DESKRIPSI:
 * File konfigurasi FreeRTOS untuk project multi-tasking pada STM32F103.
 * Project ini menjalankan dua task:
 * 1. Task 1: Mengontrol LED eksternal dan melaporkan statusnya via UART.
 * 2. Task 2: Membaca sensor jarak HC-SR04 dan melaporkan hasilnya via UART.
 * 
 * HARDWARE TARGET: STM32F103C8T6 (Blue Pill)
 * - LED Eksternal: PB0 (aktif HIGH)
 * - Sensor HC-SR04: PA1 (TRIG), PA2 (ECHO)
 * - UART: PA9 (TX), PA10 (RX) @ 115200 baud
 * ============================================================================
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ============================================================================
 * INCLUDE HEADER UNTUK DEFINISI STM32
 * ============================================================================ */
#include "stm32f1xx.h"  /* Diperlukan untuk SystemCoreClock dan definisi interrupt */

/* ============================================================================
 * BAGIAN 1: KONFIGURASI FUNDAMENTAL KERNEL
 * ============================================================================
 * Pengaturan dasar yang menentukan perilaku inti dari FreeRTOS kernel.
 * ============================================================================ */

/* ---------------------------------------------------------------------------
 * configUSE_PREEMPTION
 * ---------------------------------------------------------------------------
 * Mengaktifkan/menonaktifkan preemptive multitasking.
 * 
 * NILAI: 0 = Cooperative scheduling (task harus yield secara eksplisit)
 *        1 = Preemptive scheduling (kernel bisa interrupt task kapan saja)
 * 
 * PENTING UNTUK ABSOLUTE TIMING:
 * Dengan preemptive = 1, task dengan prioritas lebih tinggi bisa preempt
 * task yang sedang berjalan, mempengaruhi timing jika tidak dikelola.
 * vTaskDelayUntil tetap menjaga periode absolut karena menghitung
 * dari waktu bangun sebelumnya, bukan dari waktu pemanggilan.
 */
#define configUSE_PREEMPTION                    1  /* Rentang: 0-1 */

/* ---------------------------------------------------------------------------
 * configUSE_PORT_OPTIMISED_TASK_SELECTION
 * ---------------------------------------------------------------------------
 * Menggunakan instruksi CLZ (Count Leading Zeros) untuk seleksi task cepat.
 * 
 * NILAI: 0 = Algoritma generik (loop semua prioritas)
 *        1 = Menggunakan instruksi hardware CLZ (lebih cepat, Cortex-M3+)
 * 
 * Cortex-M3 (STM32F103) mendukung CLZ, jadi kita aktifkan.
 */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1  /* Rentang: 0-1 */

/* ---------------------------------------------------------------------------
 * configUSE_TICKLESS_IDLE
 * ---------------------------------------------------------------------------
 * Mode hemat daya saat idle.
 * 
 * NILAI: 0 = SysTick terus berjalan
 *        1 = SysTick dihentikan saat idle untuk hemat daya
 * 
 * PERINGATAN: Untuk aplikasi yang membutuhkan timing presisi,
 * disarankan tetap 0 agar timing konsisten.
 */
#define configUSE_TICKLESS_IDLE                 0  /* Rentang: 0-1 */

/* ---------------------------------------------------------------------------
 * configCPU_CLOCK_HZ
 * ---------------------------------------------------------------------------
 * Frekuensi clock CPU dalam Hz.
 * STM32F103C8T6 dengan HSE 8MHz dan PLL x9 = 72MHz.
 * 
 * NILAI: SystemCoreClock (variabel runtime, biasanya 72000000)
 * 
 * Digunakan untuk:
 * - Perhitungan waktu yang akurat
 * - Kalibrasi SysTick timer
 */
#define configCPU_CLOCK_HZ                      (SystemCoreClock)

/* ---------------------------------------------------------------------------
 * configTICK_RATE_HZ
 * ---------------------------------------------------------------------------
 * Frekuensi tick interrupt (berapa kali per detik scheduler dipanggil).
 * 
 * NILAI UMUM: 100-1000 Hz
 * - 100 Hz = resolusi 10ms (hemat daya, kurang presisi)
 * - 1000 Hz = resolusi 1ms (lebih presisi, lebih banyak overhead)
 * 
 * PENTING UNTUK ABSOLUTE TIMING:
 * Semakin tinggi tick rate, semakin presisi vTaskDelayUntil.
 * Dengan 1000 Hz, resolusi timing adalah 1ms.
 */
#define configTICK_RATE_HZ                      ((TickType_t)1000)  /* Rentang: 1-10000 */

/* ---------------------------------------------------------------------------
 * configMAX_PRIORITIES
 * ---------------------------------------------------------------------------
 * Jumlah level prioritas yang tersedia.
 * 
 * NILAI: 1-56 (tergantung RAM)
 * 
 * Untuk demo ini:
 * - Priority 1: Relative Delay Task (menunjukkan jitter)
 * - Priority 2: Absolute Delay Task (jitter-free)
 * - Higher priority task akan preempt lower priority
 */
#define configMAX_PRIORITIES                    8  /* Rentang: 1-56 */

/* ---------------------------------------------------------------------------
 * configMINIMAL_STACK_SIZE
 * ---------------------------------------------------------------------------
 * Ukuran stack minimum dalam WORDS (bukan bytes).
 * 
 * 1 word = 4 bytes pada ARM 32-bit
 * 128 words = 512 bytes
 * 
 * Stack digunakan untuk:
 * - Variabel lokal (buffer, counter)
 * - Context switch registers
 * - Nested function calls
 */
#define configMINIMAL_STACK_SIZE                ((uint16_t)128)  /* Rentang: 64-1024 words */

/* ---------------------------------------------------------------------------
 * configTOTAL_HEAP_SIZE
 * ---------------------------------------------------------------------------
 * Total memori heap untuk alokasi dinamis FreeRTOS.
 * 
 * STM32F103C8T6 memiliki 20KB RAM.
 * 8KB untuk heap FreeRTOS (task stacks, queues, dll)
 * 
 * Digunakan untuk:
 * - Task Control Blocks
 * - Task Stacks
 * - Queue dan Semaphore
 */
#define configTOTAL_HEAP_SIZE                   ((size_t)(8 * 1024))  /* Rentang: 1024-16384 bytes */

/* ---------------------------------------------------------------------------
 * configMAX_TASK_NAME_LEN
 * ---------------------------------------------------------------------------
 * Panjang maksimum nama task untuk debugging.
 */
#define configMAX_TASK_NAME_LEN                 16  /* Rentang: 1-32 karakter */

/* ---------------------------------------------------------------------------
 * configUSE_16_BIT_TICKS
 * ---------------------------------------------------------------------------
 * Tipe data untuk tick counter.
 * 
 * NILAI: 0 = 32-bit (4.29 milyar ticks sebelum overflow)
 *        1 = 16-bit (65535 ticks sebelum overflow)
 * 
 * Untuk ARM 32-bit, gunakan 0 (32-bit) untuk range lebih luas.
 */
#define configUSE_16_BIT_TICKS                  0  /* Rentang: 0-1 */

/* ---------------------------------------------------------------------------
 * configIDLE_SHOULD_YIELD
 * ---------------------------------------------------------------------------
 * Apakah idle task harus yield ke task dengan prioritas sama.
 * 
 * NILAI: 0 = Idle task tidak yield
 *        1 = Idle task yield ke task lain dengan prioritas yang sama
 */
#define configIDLE_SHOULD_YIELD                 1  /* Rentang: 0-1 */

/* ============================================================================
 * BAGIAN 2: FITUR SINKRONISASI
 * ============================================================================ */

/* Mutex diperlukan untuk proteksi resource (seperti UART) */
#define configUSE_MUTEXES                       1  /* Rentang: 0-1 */
#define configUSE_RECURSIVE_MUTEXES             0  /* Tidak digunakan di demo ini */
#define configUSE_COUNTING_SEMAPHORES           0  /* Tidak digunakan di demo ini */

/* ============================================================================
 * BAGIAN 3: HOOK FUNCTIONS (CALLBACK)
 * ============================================================================ */

/* Hook untuk debugging dan monitoring */
#define configUSE_IDLE_HOOK                     0  /* Hook saat idle (tidak digunakan) */
#define configUSE_TICK_HOOK                     0  /* Hook setiap tick (tidak digunakan) */
#define configUSE_MALLOC_FAILED_HOOK            1  /* Hook jika malloc gagal - PENTING! */
#define configCHECK_FOR_STACK_OVERFLOW          2  /* Deteksi stack overflow - PENTING! */

/* ============================================================================
 * BAGIAN 4: RUNTIME STATS DAN DEBUG
 * ============================================================================ */

/* Statistik runtime untuk analisis timing */
#define configGENERATE_RUN_TIME_STATS           0  /* Tidak aktif untuk demo ini */
#define configUSE_TRACE_FACILITY                0  /* Trace untuk debugging */
#define configUSE_STATS_FORMATTING_FUNCTIONS    0  /* Format stats output */

/* ============================================================================
 * BAGIAN 5: CO-ROUTINE (DEPRECATED)
 * ============================================================================ */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         2

/* ============================================================================
 * BAGIAN 6: SOFTWARE TIMER
 * ============================================================================ */

/* Timer tidak digunakan dalam demo absolute timing ini */
#define configUSE_TIMERS                        0
#define configTIMER_TASK_PRIORITY               3
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            configMINIMAL_STACK_SIZE

/* ============================================================================
 * BAGIAN 7: KONFIGURASI INTERRUPT CORTEX-M3
 * ============================================================================
 * 
 * PENTING UNTUK TIMING PRESISI:
 * Konfigurasi prioritas interrupt harus benar agar FreeRTOS
 * dapat mengelola context switch dengan tepat waktu.
 * ============================================================================ */

#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS                     __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS                     4  /* STM32F103 = 4 bit prioritas */
#endif

/*
 * Prioritas interrupt terendah (nilai numerik tertinggi)
 * STM32F103: 4 bit = 16 level (0-15)
 */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15

/*
 * Prioritas interrupt tertinggi yang bisa memanggil API FreeRTOS
 * Interrupt dengan prioritas 0-4 TIDAK BOLEH memanggil FreeRTOS API
 * Interrupt dengan prioritas 5-15 BOLEH memanggil FreeRTOS API (dengan suffix FromISR)
 */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5

#define configKERNEL_INTERRUPT_PRIORITY         (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* ============================================================================
 * BAGIAN 8: DEFINISI HANDLER INTERRUPT
 * ============================================================================ */

#define xPortPendSVHandler                      PendSV_Handler
#define vPortSVCHandler                         SVC_Handler

/* ============================================================================
 * BAGIAN 9: FUNGSI API YANG DIAKTIFKAN
 * ============================================================================
 * 
 * Hanya aktifkan fungsi yang benar-benar digunakan untuk menghemat Flash.
 * 
 * KRITIS UNTUK DEMO INI:
 * - vTaskDelayUntil - Fungsi utama untuk absolute timing
 * - vTaskDelay - Untuk perbandingan dengan relative timing
 * ============================================================================ */

#define INCLUDE_vTaskPrioritySet                0  /* Tidak mengubah prioritas */
#define INCLUDE_uxTaskPriorityGet               0  /* Tidak membaca prioritas */
#define INCLUDE_vTaskDelete                     0  /* Tidak menghapus task */
#define INCLUDE_vTaskSuspend                    0  /* Tidak suspend task */
#define INCLUDE_xResumeFromISR                  0  /* Tidak resume dari ISR */
#define INCLUDE_vTaskDelayUntil                 1  /* WAJIB - Fungsi absolute timing! */
#define INCLUDE_vTaskDelay                      1  /* WAJIB - Untuk perbandingan */
#define INCLUDE_xTaskGetSchedulerState          1  /* Cek status scheduler */
#define INCLUDE_xTaskGetCurrentTaskHandle       0  /* Tidak perlu handle task */
#define INCLUDE_uxTaskGetStackHighWaterMark     1  /* Monitor penggunaan stack */
#define INCLUDE_xTaskGetIdleTaskHandle          0  /* Tidak perlu idle handle */
#define INCLUDE_eTaskGetState                   0  /* Tidak cek state */
#define INCLUDE_xEventGroupSetBitFromISR        0  /* Tidak pakai event group */
#define INCLUDE_xTimerPendFunctionCall          0  /* Tidak pakai timer */
#define INCLUDE_xTaskAbortDelay                 0  /* Tidak abort delay */
#define INCLUDE_xTaskGetHandle                  0  /* Tidak cari handle by name */
#define INCLUDE_xTaskResumeFromISR              0  /* Tidak resume dari ISR */

/* ============================================================================
 * BAGIAN 10: ASSERTIONS DAN ERROR HANDLING
 * ============================================================================ */

#define configASSERT(x) if((x) == 0) { taskDISABLE_INTERRUPTS(); for(;;); }

/* ============================================================================
 * BAGIAN 11: KONFIGURASI HARDWARE APLIKASI
 * ============================================================================
 * Konfigurasi khusus untuk demonstrasi Absolute Timing Control
 * ============================================================================ */

/* ---------------------------------------------------------------------------
 * KONFIGURASI LED INDICATOR
 * ---------------------------------------------------------------------------
 * Menggunakan LED built-in BluePill pada PC13
 * Konfigurasi Active LOW (LED ON = pin LOW)
 */
#define LED_GPIO_PORT                           GPIOC
#define LED_GPIO_PIN                            GPIO_PIN_13
#define LED_ACTIVE_LOW                          1  /* 1 = aktif LOW, 0 = aktif HIGH */

/* ---------------------------------------------------------------------------
 * KONFIGURASI UART DEBUG
 * ---------------------------------------------------------------------------
 * USART1 untuk menampilkan perbandingan timing
 */
#define DEBUG_UART_INSTANCE                     USART1
#define DEBUG_UART_BAUDRATE                     115200
#define DEBUG_UART_TX_PORT                      GPIOA
#define DEBUG_UART_TX_PIN                       GPIO_PIN_9
#define DEBUG_UART_RX_PORT                      GPIOA
#define DEBUG_UART_RX_PIN                       GPIO_PIN_10

#endif /* FREERTOS_CONFIG_H */
