/**
 * @file    uart.c
 * @brief   Target UART via hardware USART2 (PA2/PA3)
 *
 * ESP32 mode: full TX+RX on USART2 (routed to header by ADG mux).
 * DAPLink mode: RX-only optional CDC (TX pin free for other use; SWCLK is PB13).
 */
#include "string.h"

#include "stm32f1xx.h"
#include "uart.h"
#include "gpio.h"
#include "util.h"
#include "circ_buf.h"
#include "IO_Config.h"
#include "esp32_autoload.h"

#define CDC_UART                     USART2
#define CDC_UART_ENABLE()            __HAL_RCC_USART2_CLK_ENABLE()
#define CDC_UART_IRQn                USART2_IRQn
#define CDC_UART_IRQn_Handler        USART2_IRQHandler

#define UART_PINS_PORT_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()

#define UART_TX_PORT                 TARGET_UART_TX_PORT
#define UART_TX_PIN                  TARGET_UART_TX_PIN
#define UART_RX_PORT                 TARGET_UART_RX_PORT
#define UART_RX_PIN                  TARGET_UART_RX_PIN

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

static void clear_buffers(void)
{
    circ_buf_init(&write_buffer, write_buffer_data, sizeof(write_buffer_data));
    circ_buf_init(&read_buffer, read_buffer_data, sizeof(read_buffer_data));
}

int32_t uart_initialize(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    CDC_UART->CR1 &= ~(USART_IT_TXE | USART_IT_RXNE);
    clear_buffers();
    CDC_UART_ENABLE();
    UART_PINS_PORT_ENABLE();

    /* USART2 TX PA2 — needed in ESP32 mode (muxed to header pin4) */
    if (esp32_autoload_enabled()) {
        GPIO_InitStructure.Pin = UART_TX_PIN;
        GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
        HAL_GPIO_Init(UART_TX_PORT, &GPIO_InitStructure);
    }

    /* USART2 RX PA3 — ESP header pin3 via mux; DAP optional CDC RX */
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
    CDC_UART->CR1 &= ~(USART_IT_TXE | USART_IT_RXNE);
    clear_buffers();
    return 1;
}

int32_t uart_reset(void)
{
    const uint32_t cr1 = CDC_UART->CR1;
    CDC_UART->CR1 = cr1 & ~(USART_IT_TXE | USART_IT_RXNE);
    clear_buffers();
    CDC_UART->CR1 = cr1 & ~USART_IT_TXE;
    return 1;
}

int32_t uart_set_configuration(UART_Configuration *config)
{
    UART_HandleTypeDef uart_handle;
    HAL_StatusTypeDef status;

    memset(&uart_handle, 0, sizeof(uart_handle));
    uart_handle.Instance = CDC_UART;

    configuration.Parity = UART_PARITY_NONE;
    configuration.StopBits = UART_STOP_BITS_1;
    configuration.DataBits = UART_DATA_BITS_8;
    configuration.FlowControl = UART_FLOW_CONTROL_NONE;
    configuration.Baudrate = config->Baudrate;

    uart_handle.Init.Parity = HAL_UART_PARITY_NONE;
    uart_handle.Init.StopBits = UART_STOPBITS_1;
    uart_handle.Init.WordLength = UART_WORDLENGTH_8B;
    uart_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart_handle.Init.BaudRate = config->Baudrate;
    uart_handle.Init.Mode = esp32_autoload_enabled() ? UART_MODE_TX_RX : UART_MODE_RX;

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
    return circ_buf_count_free(&write_buffer);
}

int32_t uart_write_data(uint8_t *data, uint16_t size)
{
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
