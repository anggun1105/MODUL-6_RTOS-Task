/**
 * ============================================================================
 * FILE: main.c
 * PROJECT: 03-Absolute_Timing_Control
 * * DESKRIPSI PROGRAM:
 * Program ini mendemonstrasikan perbedaan antara dua metode delay di FreeRTOS:
 * 1. vTaskDelay() - Delay RELATIF (timing tidak presisi, ada jitter)
 * 2. vTaskDelayUntil() - Delay ABSOLUT (timing presisi, bebas jitter)
 * * HARDWARE TARGET: STM32F103C8T6 (Blue Pill)
 * - CPU: 72MHz (HSE 8MHz + PLL x9)
 * - LED: PC13 (aktif LOW) - Toggle saat sampling
 * - ADC: PA0 (ADC1_CH0) - Simulasi sensor
 * - UART: PA9 (TX), PA10 (RX) @ 115200 baud
 * ============================================================================
 */

/* ============================================================================
 * INCLUDE HEADERS
 * ============================================================================ */
#include "stm32f1xx_hal.h"      /* HAL library untuk STM32F1 series */
#include "FreeRTOS.h"          /* FreeRTOS kernel utama */
#include "task.h"              /* API untuk task management */
#include "semphr.h"            /* API untuk semaphore */
#include "portmacro.h"         /* Port macros including xPortSysTickHandler */
#include <cstring>             /* C++ version of string.h for std::strlen */
#include <cstdio>              /* C++ version of stdio.h for std::sprintf */

/* ============================================================================
 * DEFINISI MACRO PIN & PERIPHERAL
 * NOTE: LED and UART macros are defined in FreeRTOSConfig.h
 * ============================================================================ */

/* ============================================================================
 * HANDLE PERIPHERAL GLOBAL
 * ============================================================================ */
UART_HandleTypeDef huart1;     /* Handle untuk USART1 - komunikasi serial debug */
extern uint32_t SystemCoreClock; /* Pastikan SystemCoreClock dapat diakses untuk delay_us */
SemaphoreHandle_t xSemaphore;  /* Semaphore untuk sinkronisasi task bergantian */

/* ============================================================================
 * DEKLARASI FUNGSI (FUNCTION PROTOTYPES)
 * ============================================================================ */
void SystemClock_Config(void);               /* Konfigurasi clock system 72MHz */
static void MX_GPIO_Init(void);              /* Inisialisasi GPIO (LED) */
static void MX_USART1_UART_Init(void);       /* Inisialisasi UART debug */

void UART_SendString(const char *str);       /* Kirim string via UART */
void delay_us(uint32_t us);                  /* Fungsi delay presisi microsecond */

#ifdef __cplusplus
extern "C" {
#endif
/* FreeRTOS port handler - declared in FreeRTOS port implementation */
extern void xPortSysTickHandler(void);

void vTask1_LED_Status(void *pvParameters);  /* Task 1: Cetak Status LED (Relatif) */
void vTask2_HCSR04(void *pvParameters);      /* Task 2: Baca Sensor HC-SR04 (Absolut) */
void vApplicationMallocFailedHook(void);     
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName); 
void SysTick_Handler(void);                  
#ifdef __cplusplus
}
#endif

/* ============================================================================
 * FUNGSI UTILITAS
 * ============================================================================ */
void UART_SendString(const char *str) {
    HAL_UART_Transmit(&huart1, (uint8_t*)str, std::strlen(str), HAL_MAX_DELAY);
}

void delay_us(uint32_t us) {
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - start) < ticks);
}

/* ============================================================================
 * IMPLEMENTASI TASK & HOOK FREERTOS 
 * ============================================================================ */
#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------------------
 * TASK 1: Menampilkan Timestamp (Menggunakan vTaskDelay - RELATIF)
 * ---------------------------------------------------------------------------- */
void vTask1_LED_Status(void *pvParameters) {
    uint32_t counter = 0;
    char buffer[128];
    
    (void)pvParameters;
    
    for(;;) {
        /* Tunggu semaphore dari Task 2 */
        if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
            HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
            uint8_t led_state = (HAL_GPIO_ReadPin(LED_GPIO_PORT, LED_GPIO_PIN) == GPIO_PIN_SET) ? 1 : 0;
            uint32_t timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
            
            std::sprintf(buffer, "[Task 1] Timestamp: %lu ms | LED: %d | Counter: %lu | Core: 0\r\n", 
                    timestamp, led_state, counter++);
            UART_SendString(buffer);
            
            /* Delay 1 detik untuk output yang lebih lambat */
            vTaskDelay(pdMS_TO_TICKS(1000));
            
            /* Beri semaphore ke Task 2 */
            xSemaphoreGive(xSemaphore);
        }
        
        /* Delay singkat untuk menghindari busy waiting */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ----------------------------------------------------------------------------
 * TASK 2: Membaca Sensor HC-SR04 (Menggunakan vTaskDelayUntil - ABSOLUT)
 * ---------------------------------------------------------------------------- */
void vTask2_HCSR04(void *pvParameters) {
    uint32_t counter = 0;
    char buffer[128];
    
    (void)pvParameters;

    for(;;) {
        /* Tunggu semaphore dari Task 1 */
        if(xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
            /* Berikan Trigger 10us pada pin PA0 */
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
            delay_us(10);
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);

            /* Tunggu sedikit untuk sensor memproses trigger */
            delay_us(100);

            /* Tunggu respons Echo HIGH di PA1 (maks timeout 50ms) */
            uint32_t timeout_start = DWT->CYCCNT;
            uint32_t timeout_ticks = 50000 * (SystemCoreClock / 1000000);
            uint8_t echo_detected = 0;
            while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET) {
                if((DWT->CYCCNT - timeout_start) > timeout_ticks) {
                    echo_detected = 0;
                    break;
                }
            }
            if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET) echo_detected = 1;

            /* Mulai ukur waktu selama Echo HIGH */
            uint32_t echo_start = DWT->CYCCNT;
            while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET) {
                if((DWT->CYCCNT - echo_start) > timeout_ticks) break;
            }
            
            /* Konversi Cycle menjadi Microseconds dan hitung jarak */
            uint32_t echo_time_us = (DWT->CYCCNT - echo_start) / (SystemCoreClock / 1000000);
            float distance = (float)echo_time_us / 58.0f;
            uint32_t dist_int = (uint32_t)distance;
            uint32_t dist_frac = (uint32_t)((distance - dist_int) * 100);

            /* Cetak hasil dengan debug info */
            std::sprintf(buffer, "[Task 2] Echo detected: %d | Echo time: %lu us | Distance: %lu.%02lu cm | Counter: %lu\r\n", 
                    echo_detected, echo_time_us, dist_int, dist_frac, counter++);
            UART_SendString(buffer);

            /* Delay 1 detik untuk output yang lebih lambat */
            vTaskDelay(pdMS_TO_TICKS(1000));
            
            /* Beri semaphore ke Task 1 */
            xSemaphoreGive(xSemaphore);
        }
        
        /* Delay singkat untuk menghindari busy waiting */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ----------------------------------------------------------------------------
 * ERROR HOOKS & SYSTICK
 * ---------------------------------------------------------------------------- */
void vApplicationMallocFailedHook(void) {
    UART_SendString("\r\n!!! FATAL: Malloc gagal! Heap penuh. !!!\r\n");
    taskDISABLE_INTERRUPTS();
    for(;;) {
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
        for(volatile uint32_t i = 0; i < 100000; i++);
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    UART_SendString("\r\n!!! FATAL: Stack Overflow pada task: ");
    UART_SendString(pcTaskName);
    UART_SendString(" !!!\r\n");
    taskDISABLE_INTERRUPTS();
    for(;;) {
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
        for(volatile uint32_t i = 0; i < 50000; i++);
    }
}

void SysTick_Handler(void) {
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();
    }
}

#ifdef __cplusplus
} 
#endif

/* ============================================================================
 * FUNGSI INISIALISASI PERIPHERAL
 * ============================================================================ */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {}

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | 
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {}
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    #if LED_ACTIVE_LOW
        HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_SET);
    #else
        HAL_GPIO_WritePin(LED_GPIO_PORT, LED_GPIO_PIN, GPIO_PIN_RESET);
    #endif
    
    GPIO_InitStruct.Pin = LED_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);

    /* Pin PA0 sebagai TRIG (Output) HY-SRF05 */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Pin PA1 sebagai ECHO (Input) HY-SRF05 */
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void MX_USART1_UART_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = DEBUG_UART_TX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DEBUG_UART_TX_PORT, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = DEBUG_UART_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DEBUG_UART_RX_PORT, &GPIO_InitStruct);

    huart1.Instance = DEBUG_UART_INSTANCE;
    huart1.Init.BaudRate = DEBUG_UART_BAUDRATE;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

/* ============================================================================
 * MAIN FUNCTION
 * ============================================================================ */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    
    /* Enable DWT untuk akurasi delay_us() */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    MX_GPIO_Init();
    MX_USART1_UART_Init();

    UART_SendString("\r\n============================================================\r\n");
    UART_SendString("  Program FreeRTOS - Multi-Tasking STM32F1\r\n");
    UART_SendString("============================================================\r\n\r\n");

    /* Buat semaphore binary untuk sinkronisasi task bergantian */
    xSemaphore = xSemaphoreCreateBinary();
    if(xSemaphore == NULL) {
        UART_SendString("ERROR: Gagal membuat semaphore!\r\n");
        while(1) {
            HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
            HAL_Delay(100);
        }
    }
    
    /* Beri semaphore awal ke Task 1 agar mulai dari Task 1 */
    xSemaphoreGive(xSemaphore);

    BaseType_t result1 = xTaskCreate(vTask1_LED_Status, "LED_Task", 256, NULL, 1, NULL);
    BaseType_t result2 = xTaskCreate(vTask2_HCSR04, "HCSR04_Task", 256, NULL, 2, NULL);
    
    if(result1 != pdPASS || result2 != pdPASS) {
        UART_SendString("ERROR: Gagal membuat task! Heap tidak cukup?\r\n");
        while(1) {
            HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
            HAL_Delay(100);
        }
    }
    
    UART_SendString("Task berhasil dibuat. Memulai scheduler...\r\n\r\n");
    vTaskStartScheduler();

    UART_SendString("ERROR: Scheduler gagal start!\r\n");
    while(1) {
        HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_GPIO_PIN);
        HAL_Delay(500);
    }
}