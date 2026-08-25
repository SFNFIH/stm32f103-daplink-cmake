/**
 * @file    uart.c
 * @brief   Target UART — HW USART2 in DAP mode, soft UART in ESP32 mode
 */
#include "string.h"

#include "stm32f1xx.h"
#include "uart.h"
#include "gpio.h"
#include "util.h"
#include "circ_buf.h"
#include "IO_Config.h"
#include "esp32_autoload.h"
#include "esp32_soft_uart.h"

#define CDC_UART                     USART2
#define CDC_UART_ENABLE()            __HAL_RCC_USART2_CLK_ENABLE()
#define CDC_UART_DISABLE()           __HAL_RCC_USART2_CLK_DISABLE()
#define CDC_UART_IRQn                USART2_IRQn
#define CDC_UART_IRQn_Handler        USART2_IRQHandler

#define UART_PINS_PORT_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()

#define UART_TX_PORT                 GPIOA
#define UART_TX_PIN                  GPIO_PIN_2

#define UART_RX_PORT                 GPIOA
#define UART_RX_PIN                  GPIO_PIN_3

#define UART_CTS_PORT                GPIOA
#define UART_CTS_PIN                 GPIO_PIN_0

#define UART_RTS_PORT                GPIOA
#define UART_RTS_PIN                 GPIO_PIN_1

#define RX_OVRF_MSG         "<DAPLink:Overflow>\n"
#define RX_OVRF_MSG_SIZE    (sizeof(RX_OVRF_MSG) - 1)
#define BUFFER_SIZE         (512)

circ_buf_t write_buffer;
uint8_t write_buffer_data[BUFFER_SIZE];
circ_buf_t read_buffer;
uint8_t read_buffer_data[BUFFER_SIZE];

static UART_Configuration configuration = {
    .Baudrate = 9600,
    .DataBits = UART_DATA_BITS_8,
    .Parity = UART_PARITY_NONE,
    .StopBits = UART_STOP_BITS_1,
    .FlowControl = UART_FLOW_CONTROL_NONE,
};

extern uint32_t SystemCoreClock;

static void clear_buffers(void)
{
    circ_buf_init(&write_buffer, write_buffer_data, sizeof(write_buffer_data));
    circ_buf_init(&read_buffer, read_buffer_data, sizeof(read_buffer_data));
}

int32_t uart_initialize(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    clear_buffers();

    if (esp32_autoload_enabled()) {
        /* ESP32 mode: GPIO soft UART on PA2/PB14; PA3 is IO0 (not USART). */
        esp32_soft_uart_init();
        return 1;
    }

    /* DAPLink mode: USART2 RX-only on PA3 (PA2 is SWCLK). */
    CDC_UART->CR1 &= ~(USART_IT_TXE | USART_IT_RXNE);
    CDC_UART_ENABLE();
    UART_PINS_PORT_ENABLE();

    GPIO_InitStructure.Pin = UART_RX_PIN;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(UART_RX_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.Pin = UART_CTS_PIN;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(UART_CTS_PORT, &GPIO_InitStructure);

    HAL_GPIO_WritePin(UART_RTS_PORT, UART_RTS_PIN, GPIO_PIN_RESET);
    GPIO_InitStructure.Pin = UART_RTS_PIN;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(UART_RTS_PORT, &GPIO_InitStructure);

    NVIC_EnableIRQ(CDC_UART_IRQn);
    return 1;
}

int32_t uart_uninitialize(void)
{
    if (!esp32_autoload_enabled()) {
        CDC_UART->CR1 &= ~(USART_IT_TXE | USART_IT_RXNE);
    }
    clear_buffers();
    return 1;
}

int32_t uart_reset(void)
{
    if (!esp32_autoload_enabled()) {
        const uint32_t cr1 = CDC_UART->CR1;
        CDC_UART->CR1 = cr1 & ~(USART_IT_TXE | USART_IT_RXNE);
        clear_buffers();
        CDC_UART->CR1 = cr1 & ~USART_IT_TXE;
    } else {
        clear_buffers();
    }
    return 1;
}

int32_t uart_set_configuration(UART_Configuration *config)
{
    configuration.Parity = UART_PARITY_NONE;
    configuration.StopBits = UART_STOP_BITS_1;
    configuration.DataBits = UART_DATA_BITS_8;
    configuration.FlowControl = UART_FLOW_CONTROL_NONE;
    configuration.Baudrate = config->Baudrate;

    if (esp32_autoload_enabled()) {
        esp32_soft_uart_set_baudrate(configuration.Baudrate);
        clear_buffers();
        return 1;
    }

    UART_HandleTypeDef uart_handle;
    HAL_StatusTypeDef status;

    memset(&uart_handle, 0, sizeof(uart_handle));
    uart_handle.Instance = CDC_UART;
    uart_handle.Init.Parity = HAL_UART_PARITY_NONE;
    uart_handle.Init.StopBits = UART_STOPBITS_1;
    uart_handle.Init.WordLength = UART_WORDLENGTH_8B;
    uart_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart_handle.Init.BaudRate = config->Baudrate;
    uart_handle.Init.Mode = UART_MODE_RX; /* PA2 is SWCLK — RX only */

    CDC_UART->CR1 &= ~(USART_IT_TXE | USART_IT_RXNE);
    clear_buffers();

    status = HAL_UART_DeInit(&uart_handle);
    util_assert(HAL_OK == status);
    status = HAL_UART_Init(&uart_handle);
    util_assert(HAL_OK == status);
    (void)status;

    CDC_UART->CR1 |= USART_IT_RXNE;
    return 1;
}

int32_t uart_get_configuration(UART_Configuration *config)
{
    config->Baudrate = configuration.Baudrate;
    config->DataBits = configuration.DataBits;
    config->Parity = configuration.Parity;
    config->StopBits = configuration.StopBits;
    config->FlowControl = UART_FLOW_CONTROL_NONE;
    return 1;
}

void uart_set_control_line_state(uint16_t ctrl_bmp)
{
    esp32_autoload_set_control_lines(ctrl_bmp);
}

int32_t uart_write_free(void)
{
    if (esp32_autoload_enabled()) {
        return 64; /* soft TX is synchronous */
    }
    return circ_buf_count_free(&write_buffer);
}

int32_t uart_write_data(uint8_t *data, uint16_t size)
{
    if (esp32_autoload_enabled()) {
        return (int32_t)esp32_soft_uart_write(data, size);
    }

    uint32_t cnt = circ_buf_write(&write_buffer, data, size);
    CDC_UART->CR1 |= USART_IT_TXE;
    return cnt;
}

int32_t uart_read_data(uint8_t *data, uint16_t size)
{
    return circ_buf_read(&read_buffer, data, size);
}

void CDC_UART_IRQn_Handler(void)
{
    if (esp32_autoload_enabled()) {
        return;
    }

    const uint32_t sr = CDC_UART->SR;

    if (sr & USART_SR_RXNE) {
        uint8_t dat = CDC_UART->DR;
        uint32_t free = circ_buf_count_free(&read_buffer);
        if (free > RX_OVRF_MSG_SIZE) {
            circ_buf_push(&read_buffer, dat);
        } else if (RX_OVRF_MSG_SIZE == free) {
            circ_buf_write(&read_buffer, (uint8_t *)RX_OVRF_MSG, RX_OVRF_MSG_SIZE);
        }
    }

    if (sr & USART_SR_TXE) {
        if (circ_buf_count_used(&write_buffer) > 0) {
            CDC_UART->DR = circ_buf_pop(&write_buffer);
        } else {
            CDC_UART->CR1 &= ~USART_IT_TXE;
        }
    }
}
