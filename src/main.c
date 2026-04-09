#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c.h"

// ---------------------------------------------------------
// CONFIG
// ---------------------------------------------------------
#define HEARTBEAT_PIN 10
#define NUM_BUZZERS 4
int NOTE_GAP_MS = 15;

const int BUZZ_PINS[NUM_BUZZERS] = {4, 5, 6, 7};
const ledc_timer_t TIMERS[NUM_BUZZERS] = {LEDC_TIMER_0, LEDC_TIMER_1, LEDC_TIMER_2, LEDC_TIMER_3};
const ledc_channel_t CHANNELS[NUM_BUZZERS] = {LEDC_CHANNEL_0, LEDC_CHANNEL_1, LEDC_CHANNEL_2, LEDC_CHANNEL_3};

// --- I2C SLAVE CONFIG ---
#define I2C_SLAVE_SDA_IO        8
#define I2C_SLAVE_SCL_IO        9
#define I2C_SLAVE_NUM           0
#define I2C_SLAVE_ADDR          0x20
#define I2C_SLAVE_RX_BUF_LEN    256
#define I2C_SLAVE_TX_BUF_LEN    256

// ---------------------------------------------------------
// DATA STRUCTURES
// ---------------------------------------------------------
typedef struct {
    int frequency;
    int duration;
} note_t;

typedef struct {
    const note_t *voice1;
    int voice1_len;
    const note_t *voice2;
    int voice2_len;
    int base_time_ms;
} song_t;

// ---------------------------------------------------------
// SONG 1: TRUMPET & TROMBONE
// ---------------------------------------------------------
const note_t TRUMPET[] = {
    {2794,3}, {3729,3}, {3520,3}, {3136,3}, {2794,3}, {4699,2}, {4435,1}, {4699,6},
    {2794,3}, {3729,3}, {3520,3}, {3729,3}, {4978,2}, {4978,3}, {4699,1}, {4978,1}, {0,5},
    {4699,3}, {4186,3}, {5588,3}, {4699,3}, {4186,3}, {3729,2}, {4186,1}, {4699,1}, {0,5},
    {6272,2}, {6272,1}, {5588,3}, {4978,2}, {4978,1}, {4699,3}, {6272,2}, {6272,1}, {5588,2}, {5588,1}, {4978,2}, {4978,1}, {4699,3},
    {5588,3}, {4699,2}, {3729,1}, {4186,3}, {5588,3}, {3729,7}, {0,5}
};

const note_t TROMBONE[] = {
    {1397,2}, {1397,1}, {1568,2}, {1760,1}, {1865,2}, {1760,1}, {1568,2}, {1760,1}, {1865,3}, {1397,3}, {1865,1}, {0,1},
    {2349,1}, {2794,2}, {2349,1}, {2794,6}, {2794,3}, {2217,3}, {2489,3}, {2349,3}, {2489,1}, {0,5},
    {1397,3}, {1480,3}, {1568,2}, {1661,1}, {1760,2}, {2093,1}, {1865,2}, {1760,1}, {1865,2}, {2093,1}, {2349,1}, {0,2},
    {2794,3}, {2489,1}, {0,2}, {2349,3}, {2489,1}, {0,2}, {2349,3}, {2489,2}, {2489,1}, {2349,3}, {2489,2}, {2489,1},
    {2349,3}, {1865,1}, {1760,1}, {1568,1}, {1397,2}, {2349,1}, {2489,2}, {2349,1}, {2489,3}, {1865,7}, {0,5}
};

const song_t SONG1 = {
    .voice1 = TRUMPET, .voice1_len = sizeof(TRUMPET) / sizeof(note_t),
    .voice2 = TROMBONE, .voice2_len = sizeof(TROMBONE) / sizeof(note_t),
    .base_time_ms = 133
};

// ---------------------------------------------------------
// SONG 2: FRENCH HORN DUET
// ---------------------------------------------------------
const note_t HORN_1[] = {
    {2349, 2}, {2637, 2}, {2794, 3}, {3136, 1}, {3520, 2}, {3136, 2}, {2794, 2}, {2637, 2}, 
    {2794, 2}, {3136, 2}, {3520, 2}, {3136, 3}, {0, 1}, {2794, 1}, {3136, 1}, {3520, 3}, 
    {3136, 1}, {2794, 2}, {2637, 2}, {2794, 2}, {2637, 2}, {2349, 3}, {2637, 1}, {2093, 2}, 
    {2349, 1}, {2349, 1}, {0, 1}, {2093, 1}, {1760, 2}, {0, 3}, {1760, 1}, {2349, 1}, 
    {2637, 1}, {2794, 3}, {2637, 1}, {2794, 2}, {3136, 2}, {2794, 2}, {3136, 2}, {3520, 3}, 
    {3136, 1}, {2794, 2}, {2349, 4}, {2349, 1}, {2637, 1}, {2794, 2}, {3136, 2}, {3520, 2}, 
    {3951, 2}, {2349, 3}, {3136, 1}, {2794, 3}, {3136, 1}, {2637, 2}, {2349, 3}, {0, 3}
};

const note_t HORN_2[] = {
    {0, 2}, {0, 2}, {1760, 6}, {2093, 4}, {1568, 2}, {1760, 2}, {1568, 2}, {1760, 2}, 
    {2093, 3}, {0, 3}, {2093, 3}, {1865, 1}, {1865, 2}, {1760, 2}, {1760, 2}, {1760, 2}, 
    {1397, 3}, {1568, 1}, {1568, 2}, {1397, 1}, {1397, 1}, {0, 1}, {1319, 1}, {1175, 2}, 
    {1760, 1}, {1760, 1}, {0, 2}, {1175, 1}, {1319, 1}, {1397, 6}, {1568, 4}, {2093, 2}, 
    {2093, 6}, {1760, 4}, {1865, 1}, {1760, 1}, {1760, 2}, {1568, 2}, {1397, 2}, {1568, 2}, 
    {1865, 4}, {1760, 4}, {1760, 2}, {1760, 3}, {0, 3}
};

const song_t SONG2 = {
    .voice1 = HORN_1, .voice1_len = sizeof(HORN_1) / sizeof(note_t),
    .voice2 = HORN_2, .voice2_len = sizeof(HORN_2) / sizeof(note_t),
    .base_time_ms = 160 
};
// ---------------------------------------------------------
// SONG 3: CHARGE (ALL BUZZERS)
// ---------------------------------------------------------
const note_t CHARGE[] = {
    {1568,1}, {2093,1}, {2637,1}, {3136,2}, {2637,1}, {3136,3}, {0, 6}
};

const song_t SONG3 = {
    .voice1 = CHARGE,
    .voice1_len = sizeof(CHARGE) / sizeof(note_t),
    .voice2 = CHARGE,   // same line for both
    .voice2_len = sizeof(CHARGE) / sizeof(note_t),
    .base_time_ms = 180
};

// --- GLOBAL STATE ---
const song_t *current_song = &SONG1;
volatile bool is_playing = false;     // Default to silent on boot
volatile bool reset_song_idx = false; // Flag to restart the song from the beginning

// =========================================================================
// HARDWARE SETUP & MUSIC ENGINE
// =========================================================================
void init_buzzers() {
    for(int i = 0; i < NUM_BUZZERS; i++) {
        ledc_timer_config_t timer = {
            .speed_mode = LEDC_LOW_SPEED_MODE, .duty_resolution = LEDC_TIMER_8_BIT,
            .timer_num = TIMERS[i], .freq_hz = 1000, .clk_cfg = LEDC_AUTO_CLK
        };
        ledc_timer_config(&timer);

        ledc_channel_config_t channel = {
            .speed_mode = LEDC_LOW_SPEED_MODE, .channel = CHANNELS[i],
            .timer_sel = TIMERS[i], .intr_type = LEDC_INTR_DISABLE,
            .gpio_num = BUZZ_PINS[i], .duty = 0, .hpoint = 0
        };
        ledc_channel_config(&channel);
    }
}

void play_note(int *buzzer_ids, int count, int freq, int duty) {
    for(int i = 0; i < count; i++) {
        int idx = buzzer_ids[i];
        if(freq == 0) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, CHANNELS[idx], 0);
        } else {
            ledc_set_freq(LEDC_LOW_SPEED_MODE, TIMERS[idx], freq);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, CHANNELS[idx], duty);
        }
        ledc_update_duty(LEDC_LOW_SPEED_MODE, CHANNELS[idx]);
    }
}

void music_task(void *pvParameter) {
    int voice1_buzzers[] = {0, 1};
    int voice2_buzzers[] = {2, 3};
    int all_buzzers[] = {0, 1, 2, 3};
    
    int v1_idx = 0; 
    int v2_idx = 0;
    int v1_rem = 0; 
    int v2_rem = 0;

    while(1) {
        if (!is_playing) { // if commanded to stop, mute everything and wait
            play_note(all_buzzers, 4, 0, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if (reset_song_idx) { // if I2C commanded a new song, reset counters
            v1_idx = 0; 
            v2_idx = 0;
            v1_rem = current_song->voice1[0].duration * current_song->base_time_ms;
            v2_rem = current_song->voice2[0].duration * current_song->base_time_ms;
            reset_song_idx = false;
        }

        //handles song 3 differently
        if (current_song == &SONG3) {
            int freq = current_song->voice1[v1_idx].frequency;
            play_note(all_buzzers, 4, freq, 200);
        } else {
            play_note(voice1_buzzers, 2, current_song->voice1[v1_idx].frequency, 200);
            play_note(voice2_buzzers, 2, current_song->voice2[v2_idx].frequency, 140);
        }

        int step = (v1_rem < v2_rem) ? v1_rem : v2_rem;

        if(step > NOTE_GAP_MS) {
            vTaskDelay(pdMS_TO_TICKS(step - NOTE_GAP_MS));
        }

        // Silence phase
        if (current_song == &SONG3) {
            play_note(all_buzzers, 4, 0, 0);
        } else {
            play_note(voice1_buzzers, 2, 0, 0);
            play_note(voice2_buzzers, 2, 0, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(NOTE_GAP_MS));

        v1_rem -= step;
        v2_rem -= step;

        if(v1_rem <= 0) {
            v1_idx = (v1_idx + 1) % current_song->voice1_len;
            v1_rem = current_song->voice1[v1_idx].duration * current_song->base_time_ms;

            if (v1_idx == 0) is_playing = false; // stop after one play
        }

        if(v2_rem <= 0) {
            v2_idx = (v2_idx + 1) % current_song->voice2_len;
            v2_rem = current_song->voice2[v2_idx].duration * current_song->base_time_ms;
        }
    }
}

// =========================================================================
// I2C SLAVE ENGINE
// =========================================================================
esp_err_t i2c_slave_init(void) {
    int i2c_slave_port = I2C_SLAVE_NUM;
    i2c_config_t conf_slave = {
        .sda_io_num = I2C_SLAVE_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_DISABLE, // Relying on the Master's pullups
        .scl_io_num = I2C_SLAVE_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_DISABLE, // Relying on the Master's pullups
        .mode = I2C_MODE_SLAVE,
        .slave.addr_10bit_en = 0,
        .slave.slave_addr = I2C_SLAVE_ADDR,
    };
    esp_err_t err = i2c_param_config(i2c_slave_port, &conf_slave);
    if (err != ESP_OK) return err;
    return i2c_driver_install(i2c_slave_port, conf_slave.mode, I2C_SLAVE_RX_BUF_LEN, I2C_SLAVE_TX_BUF_LEN, 0);
}

void i2c_slave_task(void *pvParameter) {
    uint8_t data[1]; // We only expect a 1-byte command from the master

    while(1) {
        // Block and wait until the Master sends us data
        int size = i2c_slave_read_buffer(I2C_SLAVE_NUM, data, 1, portMAX_DELAY);
        if (size > 0) {
            printf("[I2C] Received Command: 0x%02X\n", data[0]);

            if (data[0] == 0x00) {
                is_playing = false; // Mute
            } 
            else if (data[0] == 0x01) {
                current_song = &SONG1;
                reset_song_idx = true;
                is_playing = true;
            } 
            else if (data[0] == 0x02) {
                current_song = &SONG2;
                reset_song_idx = true;
                is_playing = true;
            }
            else if (data[0] == 0x03) {
                current_song = &SONG3;
                reset_song_idx = true;
                is_playing = true;
            }
        }
    }
}

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

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    printf("--- BUZZER SLAVE BOOTING ---\n");

    init_buzzers();
    
    if (i2c_slave_init() == ESP_OK) {
        printf("[SYSTEM] I2C Slave initialized at 0x%02X\n", I2C_SLAVE_ADDR);
        xTaskCreate(i2c_slave_task, "i2c_slave", 2048, NULL, 4, NULL);
    } else {
        printf("[ERROR] Failed to initialize I2C Slave!\n");
    }

    xTaskCreate(heartbeat_task, "heartbeat", 2048, NULL, 1, NULL);
    xTaskCreate(music_task, "music", 4096, NULL, 5, NULL);
}