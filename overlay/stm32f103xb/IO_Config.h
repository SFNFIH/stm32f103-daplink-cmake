/**
 * @file    IO_Config.h
 * @brief   STM32F103xB HIC — unified 6-pin target connector for DAPLink / ESP32
 *
 * Target header (same for both modes; select mode with PB1 at power-up):
 *
 *   1  3V3
 *   2  GND
 *   3  DIO     PB14   SWDIO (DAP)  /  UART RX soft (ESP)
 *   4  CLK/TX  PA2    SWCLK (DAP)  /  UART TX soft (ESP)
 *   5  RST     PB0    nRESET (DAP) /  EN (ESP)
 *   6  IO0/RX  PA3    UART RX CDC (DAP) /  IO0 GPIO (ESP)
 *
 * ESP32 mode does NOT use hardware USART: TX/RX are GPIO bit-bang.
 * IO0 is a plain GPIO driven by USB-CDC DTR.
 */
#ifndef __IO_CONFIG_H__
#define __IO_CONFIG_H__

#include "stm32f1xx.h"
#include "compiler.h"
#include "daplink.h"

COMPILER_ASSERT(DAPLINK_HIC_ID == DAPLINK_HIC_ID_STM32F103XB);

/* USB connect */
#define USB_CONNECT_PORT_ENABLE()    __HAL_RCC_GPIOA_CLK_ENABLE()
#define USB_CONNECT_PORT_DISABLE()   __HAL_RCC_GPIOA_CLK_DISABLE()
#define USB_CONNECT_PORT             GPIOA
#define USB_CONNECT_PIN              GPIO_PIN_15
#define USB_CONNECT_ON()             (USB_CONNECT_PORT->BSRR = USB_CONNECT_PIN)
#define USB_CONNECT_OFF()            (USB_CONNECT_PORT->BRR  = USB_CONNECT_PIN)

/* LEDs */
#define CONNECTED_LED_PORT           GPIOB
#define CONNECTED_LED_PIN            GPIO_PIN_6
#define CONNECTED_LED_PIN_Bit        6

#define POWER_EN_PIN_PORT            GPIOB
#define POWER_EN_PIN                 GPIO_PIN_15
#define POWER_EN_Bit                 15

/* ---- Unified target connector ---- */

/* Pin5: nRESET / ESP32 EN */
#define nRESET_PIN_PORT              GPIOB
#define nRESET_PIN                   GPIO_PIN_0
#define nRESET_PIN_Bit               0

/*
 * Pin4: SWCLK (DAPLink) / soft UART TX (ESP32)
 */
#define SWCLK_TCK_PIN_PORT           GPIOA
#define SWCLK_TCK_PIN                GPIO_PIN_2
#define SWCLK_TCK_PIN_Bit            2

/*
 * Pin3: SWDIO (DAPLink) / soft UART RX (ESP32)
 * Single bidirectional SWDIO pad in DAP mode.
 */
#define SWDIO_OUT_PIN_PORT           GPIOB
#define SWDIO_OUT_PIN                GPIO_PIN_14
#define SWDIO_OUT_PIN_Bit            14

#define SWDIO_IN_PIN_PORT            SWDIO_OUT_PIN_PORT
#define SWDIO_IN_PIN                 SWDIO_OUT_PIN
#define SWDIO_IN_PIN_Bit             SWDIO_OUT_PIN_Bit

/*
 * Pin6: DAP CDC RX (USART2_RX) / ESP32 IO0 (GPIO via DTR)
 */
#define TARGET_UART_RX_PORT          GPIOA
#define TARGET_UART_RX_PIN           GPIO_PIN_3

/* Status LEDs (not on target header) */
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

/*
 * Mode select (board jumper, not on target 6-pin):
 *   HIGH / floating -> DAPLink
 *   LOW  (to GND)   -> ESP32 auto-download
 */
#define MODE_SEL_PORT                GPIOB
#define MODE_SEL_PIN                 GPIO_PIN_1
#define MODE_SEL_PIN_Bit             1

#define ESP32_EN_PORT                nRESET_PIN_PORT
#define ESP32_EN_PIN                 nRESET_PIN

/* Swapped vs earlier revision: IO0 on PA3, soft RX on PB14 */
#define ESP32_IO0_PORT               TARGET_UART_RX_PORT
#define ESP32_IO0_PIN                TARGET_UART_RX_PIN

#define ESP32_UART_TX_PORT           SWCLK_TCK_PIN_PORT
#define ESP32_UART_TX_PIN            SWCLK_TCK_PIN
#define ESP32_UART_RX_PORT           SWDIO_OUT_PIN_PORT
#define ESP32_UART_RX_PIN            SWDIO_OUT_PIN

#endif
