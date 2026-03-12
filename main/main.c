#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include <hd44780.h>


// PINS and CONSTANTS

// Keypad
#define NROWS 4
#define NCOLS 4

static const gpio_num_t row_pins[NROWS] = { GPIO_NUM_4, GPIO_NUM_6, GPIO_NUM_7, GPIO_NUM_15 };
static const gpio_num_t col_pins[NCOLS] = { GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_18, GPIO_NUM_21 };

static const char keypad_map[NROWS][NCOLS] = {
    { '1','2','3','A' },
    { '4','5','6','B' },
    { '7','8','9','C' },
    { '*','0','#','D' }
};

#define ACTIVE  0
#define NOPRESS '\0'

// LCD
#define LCD_RS GPIO_NUM_38
#define LCD_E  GPIO_NUM_37
#define LCD_D4 GPIO_NUM_36
#define LCD_D5 GPIO_NUM_35
#define LCD_D6 GPIO_NUM_48
#define LCD_D7 GPIO_NUM_47

// LED and buzzer
#define LED_GPIO     GPIO_NUM_14
#define BUZZER_GPIO  GPIO_NUM_13

// Servo PWM
#define SERVO_GPIO     GPIO_NUM_5
#define LEDC_TIMER     LEDC_TIMER_0
#define LEDC_MODE      LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL   LEDC_CHANNEL_0
#define LEDC_DUTY_RES  LEDC_TIMER_13_BIT
#define LEDC_FREQUENCY 50

#define SERVO_DUTY_LOCK    600
#define SERVO_DUTY_UNLOCK  250

// System
#define LOOP_MS           20
#define DEBOUNCE_TIME_MS  40

#define CODE_LEN        4
#define MAX_ATTEMPTS    3
#define AUTO_RELOCK_MS  10000

// Global variables

static hd44780_t lcd;
static QueueHandle_t keypad_queue;


// Initialization

static void gpio_init_outputs(void)
{
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);

    gpio_reset_pin(BUZZER_GPIO);
    gpio_set_direction(BUZZER_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZZER_GPIO, 0);
}

static void lcd_init_simple(void)
{
    lcd = (hd44780_t){
        .write_cb = NULL,
        .font = HD44780_FONT_5X8,
        .lines = 2,
        .pins = {
            .rs = LCD_RS,
            .e  = LCD_E,
            .d4 = LCD_D4,
            .d5 = LCD_D5,
            .d6 = LCD_D6,
            .d7 = LCD_D7,
            .bl = HD44780_NOT_USED
        }
    };

    hd44780_init(&lcd);
    hd44780_clear(&lcd);
}

static void servo_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = SERVO_GPIO,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channel);
}

static void servo_set_duty(int duty)
{
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

static void servo_lock(void)
{
    servo_set_duty(SERVO_DUTY_LOCK);
}

static void servo_unlock(void)
{
    servo_set_duty(SERVO_DUTY_UNLOCK);
}

static void keypad_init(void)
{
    // rows = outputs
    for (int r = 0; r < NROWS; r++) {
        gpio_reset_pin(row_pins[r]);
        gpio_set_direction(row_pins[r], GPIO_MODE_OUTPUT);
        gpio_set_level(row_pins[r], !ACTIVE);
    }

    // cols = inputs with pullups
    for (int c = 0; c < NCOLS; c++) {
        gpio_reset_pin(col_pins[c]);
        gpio_set_direction(col_pins[c], GPIO_MODE_INPUT);

        if (ACTIVE == 0) {
            gpio_pullup_en(col_pins[c]);
            gpio_pulldown_dis(col_pins[c]);
        } else {
            gpio_pulldown_en(col_pins[c]);
            gpio_pullup_dis(col_pins[c]);
        }
    }
}

// LCD HELPERS

static void lcd_line(int line, const char *msg)
{
    hd44780_gotoxy(&lcd, 0, line);
    hd44780_puts(&lcd, "                ");
    hd44780_gotoxy(&lcd, 0, line);
    hd44780_puts(&lcd, msg);
}

static void lcd_show_enter(void)
{
    hd44780_clear(&lcd);
    lcd_line(0, "Enter Code:");
    lcd_line(1, "");
}

// KEYPAD SCAN

static char scan_keypad(void)
{
    for (int r = 0; r < NROWS; r++)
    {
        // all rows inactive
        for (int rr = 0; rr < NROWS; rr++) {
            gpio_set_level(row_pins[rr], !ACTIVE);
        }

        // one row active
        gpio_set_level(row_pins[r], ACTIVE);

        // read columns
        for (int c = 0; c < NCOLS; c++) {
            if (gpio_get_level(col_pins[c]) == ACTIVE) {
                return keypad_map[r][c];
            }
        }
    }

    return NOPRESS;
}


// TASK 1: KEYPAD TASK

static void keypad_task(void *arg)
{
    // 0 = WAIT_FOR_PRESS
    // 1 = DEBOUNCE
    // 2 = WAIT_FOR_RELEASE
    int k_state = 0;

    char last_key = NOPRESS;
    int debounce_ms = 0;

    while (1)
    {
        char k = scan_keypad();

        if (k_state == 0)
        {
            if (k != NOPRESS)
            {
                last_key = k;
                debounce_ms = 0;
                k_state = 1;
            }
        }
        else if (k_state == 1)
        {
            debounce_ms += LOOP_MS;

            if (debounce_ms >= DEBOUNCE_TIME_MS)
            {
                if (k == last_key && k != NOPRESS)
                {
                    xQueueSend(keypad_queue, &k, 0);
                    k_state = 2;
                }
                else
                {
                    k_state = 0;
                }
            }
        }
        else
        {
            if (k == NOPRESS)
            {
                k_state = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(LOOP_MS));
    }
}

// TASK 2: CONTROL TASK

static void control_task(void *arg)
{
    // states:
    // 0 = READY
    // 1 = CHECK
    // 2 = UNLOCKED
    // 3 = ALARM
    // 4 = WRONG_SCREEN

    int state = 0;

    char correct_code[CODE_LEN + 1] = "0000";
    char entered[CODE_LEN + 1];
    int entered_len = 0;
    entered[0] = '\0';

    int attempts = 0;
    TickType_t unlock_start = 0;

    lcd_show_enter();

    while (1)
    {
        // auto relock timing
        if (state == 2)
        {
            TickType_t now = xTaskGetTickCount();

            if ((now - unlock_start) >= pdMS_TO_TICKS(AUTO_RELOCK_MS))
            {
                servo_lock();
                gpio_set_level(LED_GPIO, 0);

                hd44780_clear(&lcd);
                lcd_line(0, "Locked");
                lcd_line(1, "");

                vTaskDelay(pdMS_TO_TICKS(600));

                lcd_show_enter();
                state = 0;
            }
        }

        char key;

        if (xQueueReceive(keypad_queue, &key, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            // ALARM state
            if (state == 3)
            {
                // ignore all input
            }

            // UNLOCKED state
            else if (state == 2)
            {
                // ignore all input while unlocked
            }

            // WRONG SCREEN state
            else if (state == 4)
            {
                // user must press * to try again
                if (key == '*')
                {
                    entered_len = 0;
                    entered[0] = '\0';
                    lcd_show_enter();
                    state = 0;
                }
            }

            // READY state
            else if (state == 0)
            {
                // digit pressed
                if (key >= '0' && key <= '9')
                {
                    if (entered_len < CODE_LEN)
                    {
                        entered[entered_len++] = key;
                        entered[entered_len] = '\0';

                        char stars[17];
                        for (int i = 0; i < entered_len; i++) {
                            stars[i] = '*';
                        }
                        stars[entered_len] = '\0';

                        lcd_line(1, stars);
                    }
                }

                // clear key
                else if (key == '*')
                {
                    entered_len = 0;
                    entered[0] = '\0';
                    lcd_line(1, "");
                }

                // enter key
                else if (key == 'D')
                {
                    state = 1;
                }
            }

            // CHECK state
            if (state == 1)
            {
                if (strcmp(entered, correct_code) == 0)
                {
                    attempts = 0;
                    entered_len = 0;
                    entered[0] = '\0';

                    servo_unlock();
                    gpio_set_level(LED_GPIO, 1);

                    hd44780_clear(&lcd);
                    lcd_line(0, "Access Granted");
                    lcd_line(1, "Unlocked");

                    unlock_start = xTaskGetTickCount();
                    state = 2;
                }
                else
                {
                    attempts++;
                    entered_len = 0;
                    entered[0] = '\0';

                    if (attempts >= MAX_ATTEMPTS)
                    {
                        gpio_set_level(BUZZER_GPIO, 1);

                        hd44780_clear(&lcd);
                        lcd_line(0, "System Locked");
                        lcd_line(1, "");

                        state = 3;
                    }
                    else
                    {
                        char msg[32];
                        snprintf(msg, sizeof(msg), "Wrong %d/%d", attempts, MAX_ATTEMPTS);

                        hd44780_clear(&lcd);
                        lcd_line(0, "Access Denied");
                        lcd_line(1, msg);

                        state = 4;
                    }
                }
            }
        }
    }
}


// APP main

void app_main(void)
{
    gpio_init_outputs();
    lcd_init_simple();
    servo_init();
    keypad_init();

    servo_lock();
    gpio_set_level(LED_GPIO, 0);
    gpio_set_level(BUZZER_GPIO, 0);

    keypad_queue = xQueueCreate(10, sizeof(char));

    xTaskCreate(keypad_task, "keypad_task", 2048, NULL, 5, NULL);
    xTaskCreate(control_task, "control_task", 4096, NULL, 6, NULL);
}