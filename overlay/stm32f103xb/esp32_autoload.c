/**
 * @file    esp32_autoload.c
 * @brief   Boot mode select + ESP32 auto-download control-line handling
 */
#include "esp32_autoload.h"
#include "IO_Config.h"
#include "stm32f1xx.h"

static bool s_esp32_mode = false;

void esp32_autoload_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Mode select: input pull-up. Tie to GND before power-up for ESP32 mode. */
    gpio.Pin = MODE_SEL_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MODE_SEL_PORT, &gpio);

    /* Allow pull-up to settle */
    for (volatile uint32_t i = 0; i < 10000; i++) {
    }

    s_esp32_mode = (HAL_GPIO_ReadPin(MODE_SEL_PORT, MODE_SEL_PIN) == GPIO_PIN_RESET);

    if (!s_esp32_mode) {
        return;
    }

    /*
     * ESP32 mode: drive EN / IO0 directly (active-low when CDC line asserted),
     * matching the classic CP2102/CH340 auto-download transistor circuit.
     * Idle both HIGH so the chip can run after flashing.
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

    /* Distinct LED hint: connected LED on in ESP32 mode */
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

    const bool dtr = (ctrl_bmp & 0x01u) != 0u; /* bit0 */
    const bool rts = (ctrl_bmp & 0x02u) != 0u; /* bit1 */

    /* Asserted => pin LOW (transistor-equivalent for esptool classic reset) */
    HAL_GPIO_WritePin(ESP32_EN_PORT, ESP32_EN_PIN, rts ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(ESP32_IO0_PORT, ESP32_IO0_PIN, dtr ? GPIO_PIN_RESET : GPIO_PIN_SET);
}
