#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#define HEARTBEAT_PIN 10
#define BUZZ_CH_NUM 4

const int BUZZ_PINS[BUZZ_CH_NUM] = {4, 5, 6, 7};

// We MUST assign a unique timer to each channel for polyphony
const ledc_timer_t BUZZ_TIMERS[BUZZ_CH_NUM] = {LEDC_TIMER_0, LEDC_TIMER_1, LEDC_TIMER_2, LEDC_TIMER_3};
const ledc_channel_t BUZZ_CHANNELS[BUZZ_CH_NUM] = {LEDC_CHANNEL_0, LEDC_CHANNEL_1, LEDC_CHANNEL_2, LEDC_CHANNEL_3};

// C Major Arpeggio (C7, E7, G7, C8)
const uint32_t TONES[BUZZ_CH_NUM] = {2093, 2637, 3135, 4186}; 

// ---------------------------------------------------------
// TASK 1: Heartbeat (Proves OS is alive)
// ---------------------------------------------------------
void heartbeat_task(void *pvParameter) {
    gpio_reset_pin(HEARTBEAT_PIN);
    gpio_set_direction(HEARTBEAT_PIN, GPIO_MODE_OUTPUT);
    while(1) {
        gpio_set_level(HEARTBEAT_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(500)); 
        gpio_set_level(HEARTBEAT_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}

// ---------------------------------------------------------
// TASK 2: Hardware Timer Polyphony
// ---------------------------------------------------------
void polyphony_task(void *pvParameter) {
    printf("--- Initializing 4 Independent LEDC Timers ---\n");
    fflush(stdout);

    // 1. Initialize the 4 Timers and Channels independently
    for(int i = 0; i < BUZZ_CH_NUM; i++) {
        // Strip the pin from JTAG default
        gpio_reset_pin(BUZZ_PINS[i]); 

        // Configure unique timer
        ledc_timer_config_t ledc_timer = {
            .speed_mode       = LEDC_LOW_SPEED_MODE,
            .duty_resolution  = LEDC_TIMER_10_BIT,
            .timer_num        = BUZZ_TIMERS[i],
            .freq_hz          = TONES[i], // Pre-load the frequency
            .clk_cfg          = LEDC_AUTO_CLK
        };
        ledc_timer_config(&ledc_timer);

        // Map channel to the unique timer and pin
        ledc_channel_config_t ledc_channel = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel    = BUZZ_CHANNELS[i],
            .timer_sel  = BUZZ_TIMERS[i],
            .intr_type  = LEDC_INTR_DISABLE,
            .gpio_num   = BUZZ_PINS[i],
            .duty       = 0, // Start silent (0% duty cycle)
            .hpoint     = 0
        };
        ledc_channel_config(&ledc_channel);
    }

    // 2. The Playback Loop
    while(1) {
        printf("Playing Hardware Arpeggio...\n");
        fflush(stdout);

        // Turn notes on one by one (50% duty cycle = 512)
        for(int i = 0; i < BUZZ_CH_NUM; i++) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZ_CHANNELS[i], 512);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZ_CHANNELS[i]);
            vTaskDelay(pdMS_TO_TICKS(500)); // The CPU is free to do other things here!
        }

        // Hold the full chord
        printf("Holding Chord...\n");
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(3000));

        // Silence all buzzers (0% duty cycle)
        for(int i = 0; i < BUZZ_CH_NUM; i++) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZ_CHANNELS[i], 0);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZ_CHANNELS[i]);
        }
        
        printf("Silence.\n");
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

// ---------------------------------------------------------
// MAIN
// ---------------------------------------------------------
void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(2000)); // Delay for manual USB connection
    printf("\n--- Coprocessor Boot Sequence ---\n");
    fflush(stdout);

    xTaskCreate(heartbeat_task, "heartbeat", 2048, NULL, 5, NULL);
    xTaskCreate(polyphony_task, "polyphony", 2048, NULL, 5, NULL);
}