/**
 * @file    IO_Config.h
 * @brief   STM32F103xB HIC — unified 6-pin header with ADG936 (or ADG904) mux
 *
 * Target header:
 *   1  3V3
 *   2  GND
 *   3  DIO     → ADG936 S1  →  SWDIO(PB14)  or  USART2_RX(PA3)
 *   4  CLK/TX  → ADG936 S2  →  SWCLK(PB13)  or  USART2_TX(PA2)
 *   5  RST     → PB0 (nRESET / EN, always)
 *   6  IO0     → PB8 (ESP32 IO0; NC in DAP mode)
 *
 * MUX_CTRL (PB7) → ADG936 CTRL (or ADG904 A0):
 *   HIGH = DAPLink paths (SWDIO/SWCLK)
 *   LOW  = ESP32 paths (UART RX/TX)
 * MODE_SEL (PB1) selects firmware mode and drives MUX_CTRL.
 */
#ifndef __IO_CONFIG_H__
#define __IO_CONFIG_H__

#include "stm32f1xx.h"
#include "compiler.h"
#include "daplink.h"

COMPILER_ASSERT(DAPLINK_HIC_ID == DAPLINK_HIC_ID_STM32F103XB);

#define USB_CONNECT_PORT_ENABLE()    __HAL_RCC_GPIOA_CLK_ENABLE()
#define USB_CONNECT_PORT_DISABLE()   __HAL_RCC_GPIOA_CLK_DISABLE()
#define USB_CONNECT_PORT             GPIOA
#define USB_CONNECT_PIN              GPIO_PIN_15
/* Active-low pull-up enable (PNP / inverted MOSFET). Official DAPLink is
 * active-high; that left D+ attached during boot and caused error -71. */
#define USB_CONNECT_ON()             (USB_CONNECT_PORT->BRR  = USB_CONNECT_PIN)
#define USB_CONNECT_OFF()            (USB_CONNECT_PORT->BSRR = USB_CONNECT_PIN)

#define CONNECTED_LED_PORT           GPIOB
#define CONNECTED_LED_PIN            GPIO_PIN_6
#define CONNECTED_LED_PIN_Bit        6

#define POWER_EN_PIN_PORT            GPIOB
#define POWER_EN_PIN                 GPIO_PIN_15
#define POWER_EN_Bit                 15

/* Pin5: nRESET / ESP32 EN (direct, no mux) */
#define nRESET_PIN_PORT              GPIOB
#define nRESET_PIN                   GPIO_PIN_0
#define nRESET_PIN_Bit               0

/* Pin4 mux A: SWCLK */
#define SWCLK_TCK_PIN_PORT           GPIOB
#define SWCLK_TCK_PIN                GPIO_PIN_13
#define SWCLK_TCK_PIN_Bit            13

/* Pin3 mux A: SWDIO (single pad) */
#define SWDIO_OUT_PIN_PORT           GPIOB
#define SWDIO_OUT_PIN                GPIO_PIN_14
#define SWDIO_OUT_PIN_Bit            14
#define SWDIO_IN_PIN_PORT            SWDIO_OUT_PIN_PORT
#define SWDIO_IN_PIN                 SWDIO_OUT_PIN
#define SWDIO_IN_PIN_Bit             SWDIO_OUT_PIN_Bit

/* Pin4 mux B / Pin3 mux B: hardware USART2 */
#define TARGET_UART_TX_PORT          GPIOA
#define TARGET_UART_TX_PIN           GPIO_PIN_2
#define TARGET_UART_RX_PORT          GPIOA
#define TARGET_UART_RX_PIN           GPIO_PIN_3

/* Pin6: ESP32 IO0 (direct) */
#define ESP32_IO0_PORT               GPIOB
#define ESP32_IO0_PIN                GPIO_PIN_8
#define ESP32_IO0_PIN_Bit            8

#define ESP32_EN_PORT                nRESET_PIN_PORT
#define ESP32_EN_PIN                 nRESET_PIN
#define ESP32_UART_TX_PORT           TARGET_UART_TX_PORT
#define ESP32_UART_TX_PIN            TARGET_UART_TX_PIN
#define ESP32_UART_RX_PORT           TARGET_UART_RX_PORT
#define ESP32_UART_RX_PIN            TARGET_UART_RX_PIN

/* Mode jumper input */
#define MODE_SEL_PORT                GPIOB
#define MODE_SEL_PIN                 GPIO_PIN_1
#define MODE_SEL_PIN_Bit             1

/* Analog switch control (ADG936 CTRL / ADG904 Ax) */
#define MUX_CTRL_PORT                GPIOB
#define MUX_CTRL_PIN                 GPIO_PIN_7
#define MUX_CTRL_PIN_Bit             7
#define MUX_CTRL_DAP_LEVEL           GPIO_PIN_SET   /* route SWD */
#define MUX_CTRL_ESP_LEVEL           GPIO_PIN_RESET /* route UART */

#define RUNNING_LED_PORT             GPIOA
#define RUNNING_LED_PIN              GPIO_PIN_9
#define RUNNING_LED_Bit              9
#define PIN_HID_LED_PORT             GPIOA
#define PIN_HID_LED                  GPIO_PIN_9
#define PIN_HID_LED_Bit              9
#define PIN_CDC_LED_PORT             GPIOA
#define PIN_CDC_LED                  GPIO_PIN_9
#define PIN_CDC_LED_Bit              9
#define PIN_MSC_LED_PORT             GPIOA
#define PIN_MSC_LED                  GPIO_PIN_9
#define PIN_MSC_LED_Bit              9

#endif
