/**
 * @file    esp32_autoload.c
 * @brief   Boot mode select + ESP32 auto-download on the unified 6-pin header
 */
#include "esp32_autoload.h"
#include "IO_Config.h"
#include "stm32f1xx.h"

static bool s_esp32_mode = false;

void esp32_autoload_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* MODE_SEL PB1: pull-up, sample once */
    gpio.Pin = MODE_SEL_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MODE_SEL_PORT, &gpio);

    for (volatile uint32_t i = 0; i < 10000; i++) {
    }

    s_esp32_mode = (HAL_GPIO_ReadPin(MODE_SEL_PORT, MODE_SEL_PIN) == GPIO_PIN_RESET);

    if (!s_esp32_mode) {
        return;
    }

    /*
     * ESP32 mode on the same 6-pin header:
     *   PB14 IO0, PB0 EN, PA2 TX, PA3 RX  (+ 3V3 / GND)
     * Emulate classic auto-download: idle EN/IO0 high.
     */
    HAL_GPIO_WritePin(ESP32_EN_PORT, ESP32_EN_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ESP32_IO0_PORT, ESP32_IO0_PIN, GPIO_PIN_SET);

    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    gpio.Pin = ESP32_EN_PIN;
    HAL_GPIO_Init(ESP32_EN_PORT, &gpio);

    gpio.Pin = ESP32_IO0_PIN;
    HAL_GPIO_Init(ESP32_IO0_PORT, &gpio);

    /* Connected LED on => ESP32 mode */
    HAL_GPIO_WritePin(CONNECTED_LED_PORT, CONNECTED_LED_PIN, GPIO_PIN_RESET);
}

bool esp32_autoload_enabled(void)
{
    return s_esp32_mode;
}

void esp32_autoload_set_control_lines(uint16_t ctrl_bmp)
{
    if (!s_esp32_mode) {
        return;
    }

    const bool dtr = (ctrl_bmp & 0x01u) != 0u;
    const bool rts = (ctrl_bmp & 0x02u) != 0u;

    /* Classic circuit: asserted RTS->EN low, asserted DTR->IO0 low */
    HAL_GPIO_WritePin(ESP32_EN_PORT, ESP32_EN_PIN, rts ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(ESP32_IO0_PORT, ESP32_IO0_PIN, dtr ? GPIO_PIN_RESET : GPIO_PIN_SET);
}
