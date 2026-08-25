/**
 * @file    esp32_soft_uart.c
 * @brief   Software UART for ESP32 download mode (GPIO bit-bang, not HW USART)
 *
 * TX: PA2  (header pin4 CLK/TX)
 * RX: PB14 (header pin3 DIO)
 * IO0 is on PA3 and is separate from the UART path.
 */
#include "esp32_soft_uart.h"
#include "esp32_autoload.h"
#include "IO_Config.h"
#include "circ_buf.h"
#include "stm32f1xx.h"

extern circ_buf_t read_buffer;
extern uint32_t SystemCoreClock;

#define SOFT_TX_PORT    ESP32_UART_TX_PORT
#define SOFT_TX_PIN     ESP32_UART_TX_PIN
#define SOFT_RX_PORT    ESP32_UART_RX_PORT
#define SOFT_RX_PIN     ESP32_UART_RX_PIN
#define SOFT_RX_EXTI_LINE (14u)

static volatile uint32_t s_bit_cycles;
static volatile uint32_t s_half_bit_cycles;

static void soft_delay_cycles(uint32_t cycles)
{
    uint32_t start = DWT->CYCCNT;
    while ((uint32_t)(DWT->CYCCNT - start) < cycles) {
    }
}

static void dwt_cycle_counter_enable(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void esp32_soft_uart_set_baudrate(uint32_t baudrate)
{
    if (baudrate < 1200u) {
        baudrate = 1200u;
    }
    if (baudrate > 460800u) {
        baudrate = 460800u;
    }
    s_bit_cycles = SystemCoreClock / baudrate;
    if (s_bit_cycles < 16u) {
        s_bit_cycles = 16u;
    }
    s_half_bit_cycles = s_bit_cycles / 2u;
}

void esp32_soft_uart_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    dwt_cycle_counter_enable();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    HAL_GPIO_WritePin(SOFT_TX_PORT, SOFT_TX_PIN, GPIO_PIN_SET);
    gpio.Pin = SOFT_TX_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SOFT_TX_PORT, &gpio);

    gpio.Pin = SOFT_RX_PIN;
    gpio.Mode = GPIO_MODE_IT_FALLING;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(SOFT_RX_PORT, &gpio);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    esp32_soft_uart_set_baudrate(115200u);
}

void esp32_soft_uart_write_byte(uint8_t byte)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    HAL_GPIO_WritePin(SOFT_TX_PORT, SOFT_TX_PIN, GPIO_PIN_RESET);
    soft_delay_cycles(s_bit_cycles);

    for (uint32_t i = 0; i < 8u; i++) {
        HAL_GPIO_WritePin(SOFT_TX_PORT, SOFT_TX_PIN,
                           (byte & 1u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        byte >>= 1;
        soft_delay_cycles(s_bit_cycles);
    }

    HAL_GPIO_WritePin(SOFT_TX_PORT, SOFT_TX_PIN, GPIO_PIN_SET);
    soft_delay_cycles(s_bit_cycles);

    if (!primask) {
        __enable_irq();
    }
}

uint32_t esp32_soft_uart_write(const uint8_t *data, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        esp32_soft_uart_write_byte(data[i]);
    }
    return size;
}

void EXTI15_10_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(SOFT_RX_PIN) == RESET) {
        return;
    }
    __HAL_GPIO_EXTI_CLEAR_IT(SOFT_RX_PIN);

    if (!esp32_autoload_enabled()) {
        return;
    }

    /* Block-sample one frame with DWT (same timebase as TX). */
    EXTI->IMR &= ~(1u << SOFT_RX_EXTI_LINE);

    soft_delay_cycles(s_half_bit_cycles); /* to mid-start — then +1 bit to mid-bit0 */
    soft_delay_cycles(s_bit_cycles);

    uint8_t value = 0;
    for (uint32_t i = 0; i < 8u; i++) {
        if (HAL_GPIO_ReadPin(SOFT_RX_PORT, SOFT_RX_PIN) == GPIO_PIN_SET) {
            value |= (uint8_t)(1u << i);
        }
        soft_delay_cycles(s_bit_cycles);
    }

    /* stop bit */
    soft_delay_cycles(s_bit_cycles);
    circ_buf_push(&read_buffer, value);

    EXTI->IMR |= (1u << SOFT_RX_EXTI_LINE);
}
