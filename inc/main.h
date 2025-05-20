/**
 * @file main.h
 * @brief Main application header for combined GPS and PM sensor project.
 * @date [Current Date]
 */

#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <stdbool.h>
#include "platform.h"      // For platform_usart_rx_async_desc_t, platform_usart_tx_bufdesc_t
#include "parsers/pms_parser.h" // For pms_parser_internal_state_t, pms_data_t
#include "parsers/nmea_parser.h" // For NMEA_PARSER_MAX_COORD_STR_LEN, NMEA_PARSER_MAX_TIME_STR_LEN
// #include "parsers/nmea_parser.h" // Or nmea_parse.h - For any GPS specific data types if needed directly here

// Application Flags (Example - to be expanded)
#define PROG_FLAG_BANNER_PENDING            (1 << 0) // Request to display the startup banner
#define PROG_FLAG_GPS_DATA_RECEIVED         (1 << 1) // Raw GPS data chunk received
#define PROG_FLAG_GPS_SENTENCE_READY        (1 << 2) // A full NMEA sentence is ready in assembly_buf
#define PROG_FLAG_GPGLL_DATA_PARSED         (1 << 3) // GPGLL data has been parsed and is ready for display
#define PROG_FLAG_PM_DATA_RECEIVED          (1 << 4) // Raw PM sensor data chunk received
#define PROG_FLAG_PM_DATA_PARSED            (1 << 5) // PM sensor data has been parsed and is ready for display
#define PROG_FLAG_CDC_TX_BUSY               (1 << 6) // CDC Terminal TX is busy
#define PROG_FLAG_COMBINED_DISPLAY_READY    (1 << 7) // Both GPS and PM data are available for combined display

// Buffer Sizes (Example - adjust as needed)
#define CDC_TX_BUF_SZ                       256
#define CDC_RX_BUF_SZ                       64
#define GPS_RX_BUF_SZ                       2048
#define PM_RX_BUF_SZ                        64          // Adjusted to 64 based on typical data frame size
#define GPS_ASSEMBLY_BUF_SZ                 512 // To assemble full NMEA sentences
#define NMEA_ASSEMBLY_BUF_SZ 512
/**
 * @brief Main application state structure.
 */
typedef struct prog_state_type {
    volatile uint32_t flags; // Bitmask of PROG_FLAG_*

    // CDC Terminal (SERCOM3)
    platform_usart_tx_bufdesc_t cdc_tx_desc[1]; // For single fragment transmissions
    char                        cdc_tx_buf[CDC_TX_BUF_SZ];
    platform_usart_rx_async_desc_t cdc_rx_desc;
    char                        cdc_rx_buf[CDC_RX_BUF_SZ];
    bool fake_data_gps;
    // GPS Module (SERCOM1)
    platform_usart_rx_async_desc_t gps_rx_desc;
    char                        gps_rx_buf[GPS_RX_BUF_SZ];
    char                        gps_assembly_buf[GPS_ASSEMBLY_BUF_SZ];
    uint16_t                    gps_assembly_len;
    // Storage for parsed GPGLL data
    char                        parsed_gps_time[16];
    char                        parsed_gps_lat[20];
    char                        parsed_gps_lon[20];
    char                        formatted_gpggl_string[NMEA_PARSER_MAX_COORD_STR_LEN * 2 + NMEA_PARSER_MAX_TIME_STR_LEN + 10]; // Buffer for the fully formatted GPGLL string

    // PM Sensor (SERCOM0)
    platform_usart_rx_async_desc_t pm_rx_desc;
    char                        pm_rx_buf[PM_RX_BUF_SZ];
    pms_parser_internal_state_t pms_parser_state; // From pms_parser.h
    pms_data_t                  latest_pms_data;  // From pms_parser.h

    // UI state
    bool                        banner_displayed; // Whether banner has been displayed this session
    bool                        is_debug;         // Debug mode toggle for displaying raw hex data

    // Button state or other shared resources
    uint16_t button_event;
    
    // Timing control for display rate limiting
    uint32_t last_display_timestamp;
    uint32_t display_interval_ms;  // Interval between displays in milliseconds

} prog_state_t;

#endif // MAIN_H
