/**
 * @file    esp32_autoload.h
 * @brief   Boot-time mode select + ESP32 auto-download (unified 6-pin header)
 *
 * MODE_SEL PB1 (pull-up, not on target header):
 *   HIGH -> DAPLink on the 6-pin header
 *   LOW  -> ESP32 auto-download on the same 6-pin header
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
