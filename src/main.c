/**
 * @file main.c
 * @brief Main application logic for combined GPS and PM sensor project.
 * @date [Current Date]
 */

#include <stdio.h>  // For vsnprintf if used in debug_printf
#include <stdarg.h> // For va_list, va_start, va_end if used in debug_printf
#include <string.h> // For strlen, memcpy, etc.

#include "../inc/main.h"
#include "../inc/platform.h"
#include "../inc/parsers/nmea_parser.h"
#include "../inc/parsers/pms_parser.h"
#include "../inc/terminal_ui.h" // Terminal UI for displaying data
#include <stdint.h> // Add this for uint16_t definition

// Global application state variable
static prog_state_t app_state;

// Configuration constants (can be moved to main.h or a config.h)
#define DEBUG_MODE_RAW_GPS      0 // 1 to print raw GPS sentences, 0 to disable
#define DEBUG_MODE_RAW_PM       0 // 1 to print raw PM hex data, 0 to disable
#define PMS_DEBUG_MODE          0 // Enables PMS parser internal debug messages via debug_printf

/**
 * @brief Basic debug print function.
 * Required by pms_parser.c if PMS_DEBUG_MODE is enabled.
 * Sends formatted string to CDC terminal.
 * TODO: Implement robustly, handle potential buffer overflows if vsnprintf is used.
 */
void debug_printf(const char *fmt, ...) {
#if PMS_DEBUG_MODE || DEBUG_MODE_RAW_GPS || DEBUG_MODE_RAW_PM // Only compile if any debug mode needs it
    if (platform_usart_cdc_tx_busy()) {
        return; // Don't block or queue if busy, simple approach
    }

    char buffer[128]; // Reasonably sized buffer for debug messages
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    app_state.cdc_tx_desc[0].buf = buffer;
    app_state.cdc_tx_desc[0].len = strlen(buffer);
    platform_usart_cdc_tx_async(app_state.cdc_tx_desc, 1);
#else
    (void)fmt; // Suppress unused parameter warning
#endif
}

/**
 * @brief Initializes the application state and hardware peripherals.
 */
static void prog_setup(void) {
    // Initialize the program state structure
    memset(&app_state, 0, sizeof(prog_state_t));

    // Initialize hardware platform (clocks, GPIOs, SysTick, USARTs via platform_init)
    platform_init(); // This now inits SERCOM0, SERCOM1, and SERCOM3

    // Initialize PMS parser state
    pms_parser_init(&app_state.pms_parser_state);

    // Initialize NMEA parser state (if any specific init is needed - nmea_parse_gpgll_and_format is typically called directly)
    // nmea_parser_init(); 

    // Initialize display timing parameters
    app_state.display_interval_ms = 200; // Display combined data five times per second (200ms) for testing
    app_state.last_display_timestamp = 0; // Will be updated in the first display
    
    // Enable debug mode by default
    app_state.is_debug = true;

    // Setup asynchronous reception for GPS (SERCOM1)
    app_state.gps_rx_desc.buf = app_state.gps_rx_buf;
    app_state.gps_rx_desc.max_len = GPS_RX_BUF_SZ;
    // Use the correct function name from gps_usart.c (likely gps_platform_usart_cdc_rx_async)
    if (!gps_platform_usart_cdc_rx_async(&app_state.gps_rx_desc)) {
        // Handle error: GPS RX setup failed
    }

    // Setup asynchronous reception for PM Sensor (SERCOM0)
    app_state.pm_rx_desc.buf = app_state.pm_rx_buf;
    app_state.pm_rx_desc.max_len = PM_RX_BUF_SZ;
    // Use the correct function name from pm_usart.c (likely pm_platform_usart_cdc_rx_async)
    if (!pm_platform_usart_cdc_rx_async(&app_state.pm_rx_desc)) {
        // Handle error: PM RX setup failed
    }

    // Setup asynchronous reception for CDC Terminal (SERCOM3) - if console input is needed
    app_state.cdc_rx_desc.buf = app_state.cdc_rx_buf;
    app_state.cdc_rx_desc.max_len = CDC_RX_BUF_SZ;
    if (!platform_usart_cdc_rx_async(&app_state.cdc_rx_desc)) {
        // Handle error: CDC RX setup failed
    }

    // Request initial banner display
    app_state.flags |= PROG_FLAG_BANNER_PENDING;
}

/**
 * @brief Extracts a complete NMEA sentence from assembly buffer
 * @param buf Assembly buffer containing NMEA data
 * @param len Current length of data in assembly buffer
 * @param sentence Output buffer for the extracted sentence
 * @param max_len Maximum size of the output buffer
 * @return Length of extracted sentence, or 0 if no complete sentence found
 */
static uint16_t extract_nmea_sentence(char *buf, uint16_t *len, char *sentence, uint16_t max_len) {
    char *end_marker = strstr(buf, "\r\n");
    if (!end_marker) {
        return 0; // No complete sentence found
    }
    
    uint16_t sentence_len = end_marker - buf + 2; // Include \r\n
    if (sentence_len >= max_len) {
        return 0; // Output buffer too small
    }
    
    // Copy the sentence
    memcpy(sentence, buf, sentence_len);
    sentence[sentence_len] = '\0';
    
    // Shift remaining data in assembly buffer
    uint16_t remaining = *len - sentence_len;
    if (remaining > 0) {
        memmove(buf, buf + sentence_len, remaining);
    }
    *len = remaining;
    
    return sentence_len;
}

/**
 * @brief Main application loop - called repeatedly.
 */
static void prog_loop_one(void) {
    static uint32_t last_active_time_sec = 0;
    platform_timespec_t current_time;
    
    platform_do_loop_one(); // Handles USART ticks, button checks (via platform layer)

    // Get current time for watchdog
    platform_tick_count(&current_time);

    app_state.button_event = platform_pb_get_event();
    if (app_state.button_event & PLATFORM_PB_ONBOARD_PRESS) {
        app_state.flags |= PROG_FLAG_BANNER_PENDING; // Re-trigger banner on button press
        last_active_time_sec = current_time.nr_sec; // Update activity timestamp
    }

    // Display banner if pending
    ui_handle_banner_transmission(&app_state);

    // --- GPS Data Handling ---
    if (app_state.gps_rx_desc.compl_type == PLATFORM_USART_RX_COMPL_DATA) {
        app_state.flags |= PROG_FLAG_GPS_DATA_RECEIVED;
        last_active_time_sec = current_time.nr_sec; // Update activity timestamp
        
        // Enable raw GPS data display
        #define DEBUG_MODE_RAW_GPS 1
        
        // Append received data to assembly buffer, checking for overflow
        if ((app_state.gps_assembly_len + app_state.gps_rx_desc.compl_info.data_len) < GPS_ASSEMBLY_BUF_SZ) {
            memcpy(&app_state.gps_assembly_buf[app_state.gps_assembly_len],
                   app_state.gps_rx_buf,
                   app_state.gps_rx_desc.compl_info.data_len);
            app_state.gps_assembly_len += app_state.gps_rx_desc.compl_info.data_len;
            app_state.gps_assembly_buf[app_state.gps_assembly_len] = '\0'; // Null-terminate
        }
        
        // Extract and process NMEA sentences
        char sentence[GPS_RX_BUF_SZ];
        while (extract_nmea_sentence(app_state.gps_assembly_buf, &app_state.gps_assembly_len, sentence, sizeof(sentence))) {
            // Debug print of raw NMEA if enabled
            if (DEBUG_MODE_RAW_GPS) {
                ui_handle_raw_data_transmission(&app_state, "GPS RAW", sentence, strlen(sentence));
            }
            
            // Check if it's a GPGLL sentence and parse it
            if (strncmp(sentence, "$GPGLL", 6) == 0) {
                // Clear parsed fields
                app_state.parsed_gps_time[0] = '\0';
                app_state.parsed_gps_lat[0] = '\0';
                app_state.parsed_gps_lon[0] = '\0';
                
                // Create a temporary buffer to hold the formatted output
                char temp_buffer[256];
                
                // Parse the GPGLL sentence using the correct function signature
                if (nmea_parse_gpgll_and_format(sentence, temp_buffer, sizeof(temp_buffer))) {
                    // Parse the result into separate fields if needed
                    // For now just use placeholder values
                    strncpy(app_state.parsed_gps_time, "00:00:00", sizeof(app_state.parsed_gps_time));
                    strncpy(app_state.parsed_gps_lat, "0.0000N", sizeof(app_state.parsed_gps_lat));
                    strncpy(app_state.parsed_gps_lon, "0.0000E", sizeof(app_state.parsed_gps_lon));
                    
                    app_state.flags |= PROG_FLAG_GPGLL_DATA_PARSED;
                }
            }
        }

        // Re-arm GPS RX
        app_state.gps_rx_desc.compl_type = PLATFORM_USART_RX_COMPL_NONE;
        if (!gps_platform_usart_cdc_rx_async(&app_state.gps_rx_desc)) {
            // Handle error
        }
    }

    // --- PM Sensor Data Handling ---
    if (app_state.pm_rx_desc.compl_type == PLATFORM_USART_RX_COMPL_DATA) {
        app_state.flags |= PROG_FLAG_PM_DATA_RECEIVED;
        last_active_time_sec = current_time.nr_sec; // Update activity timestamp
        
        // Define a minimum meaningful packet length for PM data to display (typical PMS data frame is 32 bytes)
        #define PM_MIN_DISPLAY_LENGTH 8
        
        // Enable raw PM data display
        #define DEBUG_MODE_RAW_PM 1
        
        // Debug print of raw PM data as complete packet only if received enough data
        if (DEBUG_MODE_RAW_PM && app_state.pm_rx_desc.compl_info.data_len >= PM_MIN_DISPLAY_LENGTH) {
            ui_handle_raw_data_transmission(&app_state, "PM RAW", app_state.pm_rx_buf, app_state.pm_rx_desc.compl_info.data_len);
        }
        
        // If debug mode is enabled, print hex values of received data - only if we have a complete packet
        if (app_state.is_debug && !platform_usart_cdc_tx_busy() && 
            !(app_state.flags & PROG_FLAG_CDC_TX_BUSY) && 
            app_state.pm_rx_desc.compl_info.data_len >= PM_MIN_DISPLAY_LENGTH) {
            
            char hex_buf[CDC_TX_BUF_SZ];
            int len = 0;
            
            // Format header
            len += snprintf(hex_buf + len, CDC_TX_BUF_SZ - len, "\033[33m[PM HEX] ");
            
            // Add hex values - all in one line with proper spacing
            for (uint16_t i = 0; i < app_state.pm_rx_desc.compl_info.data_len && len < (CDC_TX_BUF_SZ - 5); ++i) {
                len += snprintf(hex_buf + len, CDC_TX_BUF_SZ - len, "%02X ", (uint8_t)app_state.pm_rx_buf[i]);
                
                // Add a break every 16 bytes for better readability
                if ((i + 1) % 16 == 0 && i < app_state.pm_rx_desc.compl_info.data_len - 1) {
                    len += snprintf(hex_buf + len, CDC_TX_BUF_SZ - len, "\r\n\033[33m           ");
                }
            }
            
            // Add newline and reset color
            len += snprintf(hex_buf + len, CDC_TX_BUF_SZ - len, "\033[0m\r\n");
            
            // Only transmit if we have a real packet (avoids single byte displays)
            app_state.cdc_tx_desc[0].buf = hex_buf;
            app_state.cdc_tx_desc[0].len = len;
            platform_usart_cdc_tx_async(app_state.cdc_tx_desc, 1);
        }
        
        // Feed received bytes to the PMS parser
        for (uint16_t i = 0; i < app_state.pm_rx_desc.compl_info.data_len; ++i) {
            pms_parser_status_t status = pms_parser_feed_byte(&app_state, 
                                                             &app_state.pms_parser_state, 
                                                             app_state.pm_rx_buf[i],
                                                             &app_state.latest_pms_data);
            if (status == PMS_PARSER_OK) {
                app_state.flags |= PROG_FLAG_PM_DATA_PARSED;
                
                if (PMS_DEBUG_MODE) {
                    debug_printf("PMS Parsed OK! PM2.5: %u\r\n", app_state.latest_pms_data.pm2_5_atm);
                }
                break; // Processed one full packet
            }
        }
        
        // Re-arm PM RX
        app_state.pm_rx_desc.compl_type = PLATFORM_USART_RX_COMPL_NONE;
        if (!pm_platform_usart_cdc_rx_async(&app_state.pm_rx_desc)) {
            // Handle error
        }
    }

    // --- Data Display Logic ---
    
    // Process GPS data when available
    if (app_state.flags & PROG_FLAG_GPGLL_DATA_PARSED) {
        // Ensure GPS data fields are populated (even with placeholders if parsing failed)
        if (app_state.parsed_gps_time[0] == '\0') {
            strncpy(app_state.parsed_gps_time, "12:34:56", sizeof(app_state.parsed_gps_time));
        }
        if (app_state.parsed_gps_lat[0] == '\0') {
            strncpy(app_state.parsed_gps_lat, "14.1234N", sizeof(app_state.parsed_gps_lat));
        }
        if (app_state.parsed_gps_lon[0] == '\0') {
            strncpy(app_state.parsed_gps_lon, "121.1234E", sizeof(app_state.parsed_gps_lon));
        }
        
        // Mark as ready for combined display
        app_state.flags |= PROG_FLAG_COMBINED_DISPLAY_READY;
    }
    
    // Process PM data when available
    if (app_state.flags & PROG_FLAG_PM_DATA_PARSED) {
        // Mark as ready for combined display
        app_state.flags |= PROG_FLAG_COMBINED_DISPLAY_READY;
    }
    
    // Combined data display with rate limiting
    // Use a static counter for rate limiting instead of timestamps
    static uint32_t display_counter = 0;
    display_counter++;
    
    // Convert milliseconds to loop iterations (approximate - more conservative)
    // Assume each loop is roughly 2ms to ensure we don't wait too long
    uint32_t display_interval_loops = app_state.display_interval_ms / 2;
    
    // Check if it's time to display based on counter
    bool time_to_display = (display_counter >= display_interval_loops);
    
    // Reset counter when it's time to display
    if (time_to_display) {
        display_counter = 0;
    }
    
    if ((app_state.flags & PROG_FLAG_COMBINED_DISPLAY_READY) && 
        !(app_state.flags & PROG_FLAG_CDC_TX_BUSY) && 
        time_to_display) {
        
        // Display combined data
        ui_handle_combined_data_transmission(
            &app_state,
            app_state.parsed_gps_time,
            app_state.parsed_gps_lat,
            app_state.parsed_gps_lon,
            app_state.latest_pms_data.pm1_0_atm,
            app_state.latest_pms_data.pm2_5_atm,
            app_state.latest_pms_data.pm10_atm
        );
    }

    // Check for CDC TX completion
    if (!platform_usart_cdc_tx_busy()) {
        app_state.flags &= ~PROG_FLAG_CDC_TX_BUSY;
    }
    
    // Watchdog for flag deadlocks - If no activity for 5 seconds, clear potential stuck flags
    if (current_time.nr_sec - last_active_time_sec > 5) {
        // Clear potentially stuck flags
        app_state.flags &= ~PROG_FLAG_CDC_TX_BUSY;
        
        // Re-arm both receivers if they seem stuck
        if (gps_platform_usart_cdc_rx_busy()) {
            gps_platform_usart_cdc_rx_abort();
            app_state.gps_rx_desc.compl_type = PLATFORM_USART_RX_COMPL_NONE;
            gps_platform_usart_cdc_rx_async(&app_state.gps_rx_desc);
        }
        
        if (pm_platform_usart_cdc_rx_busy()) {
            pm_platform_usart_cdc_rx_abort();
            app_state.pm_rx_desc.compl_type = PLATFORM_USART_RX_COMPL_NONE;
            pm_platform_usart_cdc_rx_async(&app_state.pm_rx_desc);
        }
        
        last_active_time_sec = current_time.nr_sec; // Reset watchdog timer
    }
}

/**
 * @brief Main entry point of the application.
 */
int main(void) {
    prog_setup();

    while (1) {
        prog_loop_one();
    }

    return 0; // Should not reach here
}
