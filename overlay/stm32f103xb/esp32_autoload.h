/**
 * @file    esp32_autoload.h
 * @brief   Boot mode + ADG mux + ESP32 auto-download control lines
 *
 * MODE_SEL PB1 (pull-up): HIGH=DAPLink, LOW=ESP32
 * MUX_CTRL PB7 follows mode for ADG936/ADG904 signal switching.
 */
#ifndef ESP32_AUTOLOAD_H
#define ESP32_AUTOLOAD_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void esp32_autoload_init(void);
bool esp32_autoload_enabled(void);
void esp32_autoload_set_control_lines(uint16_t ctrl_bmp);

#ifdef __cplusplus
}
#endif

#endif
