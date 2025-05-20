/**
 * @file terminal_ui.h
 * @brief Terminal user interface for combined GPS and PM sensor project.
 * @date [Current Date]
 */

#ifndef TERMINAL_UI_H
#define TERMINAL_UI_H

#include <stdbool.h>
#include <stddef.h> // For size_t
#include <stdint.h> // For uint16_t

// Forward declaration of prog_state_t to avoid circular dependencies with main.c
struct prog_state_type;
extern const char ANSI_RESET[];
extern const char ANSI_MAGENTA[];
extern const char ANSI_YELLOW[];
/**
 * @brief Handles the transmission of the application banner to the terminal.
 *
 * @param ps Pointer to the program state structure.
 */
void ui_handle_banner_transmission(struct prog_state_type *ps);

/**
 * @brief Handles the transmission of GPS data in a scrolling format.
 *
 * @param ps Pointer to the program state structure.
 * @param time_str The parsed time string.
 * @param lat_str The parsed latitude string.
 * @param lon_str The parsed longitude string.
 * @param raw_sentence_buf Buffer that held the raw NMEA sentence (to be cleared if successful).
 * @return true if transmission was successfully initiated, false otherwise.
 */
bool ui_handle_gps_data_transmission(struct prog_state_type *ps,
                                    const char *time_str,
                                    const char *lat_str, 
                                    const char *lon_str,
                                    char *raw_sentence_buf);

/**
 * @brief Handles the transmission of PM sensor data in a scrolling format.
 *
 * @param ps Pointer to the program state structure.
 * @param pm1_0 PM1.0 value in μg/m³.
 * @param pm2_5 PM2.5 value in μg/m³.
 * @param pm10 PM10 value in μg/m³.
 * @return true if transmission was successfully initiated, false otherwise.
 */
bool ui_handle_pm_data_transmission(struct prog_state_type *ps,
                                   uint16_t pm1_0,
                                   uint16_t pm2_5,
                                   uint16_t pm10);

/**
 * @brief Handles combined transmission of GPS and PM data in a single line.
 * 
 * @param ps Pointer to the program state structure.
 * @param time_str The parsed time string.
 * @param lat_str The parsed latitude string.
 * @param lon_str The parsed longitude string.
 * @param pm1_0 PM1.0 value in ug/m3.
 * @param pm2_5 PM2.5 value in ug/m3.
 * @param pm10 PM10 value in ug/m3.
 * @return true if transmission was successfully initiated, false otherwise.
 */
bool ui_handle_combined_data_transmission(struct prog_state_type *ps,
                                         const char *time_str,
                                         const char *lat_str,
                                         const char *lon_str,
                                         uint16_t pm1_0,
                                         uint16_t pm2_5,
                                         uint16_t pm10);

/**
 * @brief Handles the transmission of raw data (for debugging).
 *
 * @param ps Pointer to the program state structure.
 * @param prefix Optional prefix to identify the source of the raw data (e.g., "GPS" or "PM").
 * @param raw_data_str Pointer to the raw data string.
 * @param raw_data_len Length of the raw data string.
 * @return true if transmission was successfully initiated, false otherwise.
 */
bool ui_handle_raw_data_transmission(struct prog_state_type *ps,
                                     const char *prefix,
                                     const char *raw_data_str,
                                     size_t raw_data_len);

#endif // TERMINAL_UI_H 