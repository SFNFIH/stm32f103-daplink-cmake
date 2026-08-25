/**
 * @file    esp32_soft_uart.h
 * @brief   Bit-bang UART for ESP32 mode (TX=PA2, RX=PB14; not hardware USART)
 */
#ifndef ESP32_SOFT_UART_H
#define ESP32_SOFT_UART_H

#include <stdint.h>
#include "uart.h"

#ifdef __cplusplus
extern "C" {
#endif

void esp32_soft_uart_init(void);
void esp32_soft_uart_set_baudrate(uint32_t baudrate);
void esp32_soft_uart_write_byte(uint8_t byte);
uint32_t esp32_soft_uart_write(const uint8_t *data, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
