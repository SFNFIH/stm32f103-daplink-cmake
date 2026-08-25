/**
 * @file    esp32_autoload.h
 * @brief   Boot-time mode select + ESP32 auto-download (DTR/RTS -> EN/IO0)
 *
 * Mode pin (default PB1, pull-up):
 *   HIGH / floating -> DAPLink (CMSIS-DAP + CDC UART + MSD)
 *   LOW  (to GND)   -> ESP32 auto-download (CDC UART + classic DTR/RTS reset)
 */
#ifndef ESP32_AUTOLOAD_H
#define ESP32_AUTOLOAD_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Sample MODE_SEL and configure EN/IO0 when ESP32 mode is selected. Call from gpio_init(). */
void esp32_autoload_init(void);

/** True when MODE_SEL was low at boot (ESP32 auto-download mode). */
bool esp32_autoload_enabled(void);

/**
 * Apply USB CDC control lines (bit0=DTR, bit1=RTS).
 * Emulates the classic ESP DevKit transistor circuit:
 *   RTS asserted -> EN  driven LOW
 *   DTR asserted -> IO0 driven LOW
 * No-op in DAPLink mode.
 */
void esp32_autoload_set_control_lines(uint16_t ctrl_bmp);

#ifdef __cplusplus
}
#endif

#endif /* ESP32_AUTOLOAD_H */
