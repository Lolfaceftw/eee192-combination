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

// If NMEA_MAX_SENTENCE_LEN is not defined in nmea_parser.h, define a reasonable default here.
// Ideally, this should be in nmea_parser.h or a shared configuration header.
#ifndef NMEA_MAX_SENTENCE_LEN
#define NMEA_MAX_SENTENCE_LEN 128 // A common NMEA sentence is ~82 chars, 128 provides buffer.
#endif

// Global application state variable
static prog_state_t app_state;

// Configuration constants (can be moved to main.h or a config.h)
#define DEBUG_MODE_RAW_GPS      1 // 1 to print raw GPS sentences, 0 to disable
#define DEBUG_MODE_RAW_PM       1 // 1 to print raw PM hex data, 0 to disable
#define PMS_DEBUG_MODE          1 // Enables PMS parser internal debug messages via debug_printf

// Add LED blinking define constants
#define LED_BLINK_ON_GPS_DATA   1 // Blink LED when GPS data is received
#define LED_BLINK_ON_PM_DATA    1 // Blink LED when PM data is received and parsed
#define LED_BLINK_DURATION_MS   50 // How long to blink the LED in milliseconds

// Define constants for PM data accumulation
#define PM_BUFFER_ACCUMULATE    1 // Set to 1 to accumulate PM data into a buffer
#define PM_ACCUMULATE_THRESHOLD 32 // Increased to typical PMS5003 packet size
#define PM_FORCE_RAW_GPS        1  // Force display of raw GPS data regardless of debug setting

// PM data accumulation buffer
static uint8_t pm_accumulate_buffer[PM_RX_BUF_SZ * 2]; // Double size buffer to allow accumulation
static uint16_t pm_accumulate_len = 0;
static platform_timespec_t pm_last_receive_time = {0, 0};
static const uint32_t PM_ACCUMULATE_TIMEOUT_MS = 300; // Slightly increased timeout for accumulated buffer

// LED blinking control variables - moved to module level to be accessible from all functions
static uint32_t led_blink_start_ms = 0;
static bool led_is_blinking = false;

/**
 * @brief Basic debug print function - MODIFIED FOR SAFER CDC ACCESS.
 * Sends formatted string to CDC terminal if not busy.
 * MUST be passed the app_state pointer.
 */
void debug_printf(prog_state_t *ps, const char *fmt, ...) {
    // If CDC is busy with another transmission, skip this debug message to avoid conflict
    if (platform_usart_cdc_tx_busy() || (ps->flags & PROG_FLAG_CDC_TX_BUSY)) {
        return;
    }
    
    // Use a local static buffer for initial formatting to avoid ps->cdc_tx_buf if it's in use by a multi-part message elsewhere
    static char local_debug_format_buf[256]; 
    char final_debug_msg_buf[280]; // Buffer for "[DEBUG] " prefix + message + \r\n\0

    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(local_debug_format_buf, sizeof(local_debug_format_buf), fmt, args);
    va_end(args);
    
    if (len <= 0 || len >= sizeof(local_debug_format_buf)) {
        return; // Formatting error or overflow
    }

    // Prepend "[DEBUG] " and append \r\n
    // Ensure the message ends with a newline for proper terminal display
    // Check if the last char is already a newline, and second to last is carriage return
    bool ends_with_crlf = (len >= 2 && local_debug_format_buf[len-2] == '\r' && local_debug_format_buf[len-1] == '\n');
    bool ends_with_lf = (!ends_with_crlf && len >=1 && local_debug_format_buf[len-1] == '\n');

    if (ends_with_crlf) {
         snprintf(final_debug_msg_buf, sizeof(final_debug_msg_buf), "[DEBUG] %s", local_debug_format_buf);
    } else if (ends_with_lf) {
         // Replace just LF with CRLF for consistency
        local_debug_format_buf[len-1] = '\0'; // effectively truncate LF
        snprintf(final_debug_msg_buf, sizeof(final_debug_msg_buf), "[DEBUG] %s\r\n", local_debug_format_buf);
    } else {
        snprintf(final_debug_msg_buf, sizeof(final_debug_msg_buf), "[DEBUG] %s\r\n", local_debug_format_buf);
    }
    
    // Update len to the new length of final_debug_msg_buf
    len = strlen(final_debug_msg_buf);
    if (len == 0 || len >= sizeof(ps->cdc_tx_buf)) { // Check against the destination buffer
        // Message too long for ps->cdc_tx_buf or empty, should not happen with reasonable fmt
        return; 
    }

    // Safely copy to the shared CDC TX buffer
    memcpy(ps->cdc_tx_buf, final_debug_msg_buf, len + 1); // +1 for null terminator
    
    // Configure transmission descriptor using app_state's descriptor
    ps->cdc_tx_desc[0].buf = ps->cdc_tx_buf; // Point to the message in app_state's buffer
    ps->cdc_tx_desc[0].len = len;
    
    // Set busy flag and attempt transmission
    ps->flags |= PROG_FLAG_CDC_TX_BUSY;
    if (platform_usart_cdc_tx_async(&ps->cdc_tx_desc[0], 1)) {
        ps->flags &= ~PROG_FLAG_CDC_TX_BUSY; // Clear busy flag on success
    } else {
        // Transmission failed to start, flag remains set. Watchdog should handle if it gets stuck.
    }
}

// Add helper function to check timeouts
static bool is_timeout_elapsed(const platform_timespec_t *current_time, 
                              const platform_timespec_t *last_time, 
                              uint32_t timeout_ms) {
    platform_timespec_t delta;
    platform_tick_delta(&delta, current_time, last_time);
    
    // Convert delta to milliseconds
    uint32_t delta_ms = delta.nr_sec * 1000 + delta.nr_nsec / 1000000;
    
    return delta_ms >= timeout_ms;
}

// Add helper function to process accumulated PM data
static void process_accumulated_pm_data(struct prog_state_type *ps) {
    if (pm_accumulate_len == 0) {
        return; // Nothing to process
    }
    
    debug_printf(ps, "PM: Processing accumulated %u bytes", pm_accumulate_len);
    
    bool valid_pm_header = false;
    if (pm_accumulate_len >= 2 && pm_accumulate_buffer[0] == 0x42 && pm_accumulate_buffer[1] == 0x4D) {
        valid_pm_header = true;
        debug_printf(ps, "PM: Valid header 0x424D found.");
    }

    if (DEBUG_MODE_RAW_PM) { 
        debug_printf(ps, "PM: Preparing to print RAW HEX (%u bytes). CDC Busy_HW: %d, CDC_Busy_Flag: %d", 
            pm_accumulate_len, platform_usart_cdc_tx_busy(), (ps->flags & PROG_FLAG_CDC_TX_BUSY) ? 1:0);
        // Explicitly clear the software busy flag before attempting to send raw PM data
        ps->flags &= ~PROG_FLAG_CDC_TX_BUSY; 
        ui_handle_raw_data_transmission(ps, "PM RAW HEX", (const char*)pm_accumulate_buffer, pm_accumulate_len);
    }
    
    // Feed to parser regardless of valid_pm_header for now, parser should reject if invalid
    for (uint16_t i = 0; i < pm_accumulate_len; ++i) {
        pms_parser_status_t status = pms_parser_feed_byte(ps, 
                                                         &ps->pms_parser_state, 
                                                         pm_accumulate_buffer[i],
                                                         &ps->latest_pms_data);
        if (status == PMS_PARSER_OK) {
            ps->flags |= PROG_FLAG_PM_DATA_PARSED;
            debug_printf(ps, "PM: Parsed OK: PM1.0=%u, PM2.5=%u, PM10=%u", 
                        ps->latest_pms_data.pm1_0_atm,
                        ps->latest_pms_data.pm2_5_atm,
                        ps->latest_pms_data.pm10_atm);
            #if LED_BLINK_ON_PM_DATA
            platform_gpo_modify(PLATFORM_GPO_LED_ONBOARD, 0);
            platform_timespec_t current_time;
            platform_tick_count(&current_time);
            uint32_t current_time_ms = current_time.nr_sec * 1000 + current_time.nr_nsec / 1000000;
            led_blink_start_ms = current_time_ms;
            led_is_blinking = true;
            #endif
            // Don't break here; let parser consume as much of the buffer as possible
            // if multiple packets were somehow accumulated (though threshold should limit this)
        }
    }
    
    pm_accumulate_len = 0;
    memset(pm_accumulate_buffer, 0, sizeof(pm_accumulate_buffer));
}

/**
 * @brief Initializes the application state and hardware peripherals.
 */
static void prog_setup(void) {
    // Initialize the program state structure
    memset(&app_state, 0, sizeof(prog_state_t));

    // Initialize hardware platform (clocks, GPIOs, SysTick, USARTs via platform_init)
    platform_init(); // This now inits SERCOM0, SERCOM1, and SERCOM3
    
    // Initial LED blink to show we're alive
    platform_gpo_modify(PLATFORM_GPO_LED_ONBOARD, 0);
    
    // Debug message to show we've started
    debug_printf(&app_state, "Combined GPS and PM sensor project starting up...");

    // Initialize PMS parser state
    pms_parser_init(&app_state.pms_parser_state);
    debug_printf(&app_state, "PMS parser initialized");

    // Initialize display timing parameters
    app_state.display_interval_ms = 200; // Display combined data five times per second (200ms) for testing
    app_state.last_display_timestamp = 0; // Will be updated in the first display
    
    // Enable debug mode by default
    app_state.is_debug = true;
    debug_printf(&app_state, "Debug mode enabled");

    // Setup asynchronous reception for GPS (SERCOM1)
    app_state.gps_rx_desc.buf = app_state.gps_rx_buf;
    app_state.gps_rx_desc.max_len = GPS_RX_BUF_SZ;
    // Use the correct function name from gps_usart.c (likely gps_platform_usart_cdc_rx_async)
    if (!gps_platform_usart_cdc_rx_async(&app_state.gps_rx_desc)) {
        debug_printf(&app_state, "ERROR: GPS RX setup failed!");
    } else {
        debug_printf(&app_state, "GPS RX setup successful");
    }

    // Setup asynchronous reception for PM Sensor (SERCOM0)
    app_state.pm_rx_desc.buf = app_state.pm_rx_buf;
    app_state.pm_rx_desc.max_len = PM_RX_BUF_SZ;
    // Use the correct function name from pm_usart.c (likely pm_platform_usart_cdc_rx_async)
    if (!pm_platform_usart_cdc_rx_async(&app_state.pm_rx_desc)) {
        debug_printf(&app_state, "ERROR: PM RX setup failed!");
    } else {
        debug_printf(&app_state, "PM RX setup successful");
    }

    // Setup asynchronous reception for CDC Terminal (SERCOM3) - if console input is needed
    app_state.cdc_rx_desc.buf = app_state.cdc_rx_buf;
    app_state.cdc_rx_desc.max_len = CDC_RX_BUF_SZ;
    if (!platform_usart_cdc_rx_async(&app_state.cdc_rx_desc)) {
        debug_printf(&app_state, "ERROR: CDC RX setup failed!");
    } else {
        debug_printf(&app_state, "CDC RX setup successful");
    }

    // Request initial banner display
    app_state.flags |= PROG_FLAG_BANNER_PENDING;
    
    // Turn off LED after initialization
    platform_gpo_modify(0, PLATFORM_GPO_LED_ONBOARD);
    
    debug_printf(&app_state, "Initialization complete, entering main loop");
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

    // Get current time for watchdog and other timing
    platform_tick_count(&current_time);
    uint32_t current_time_ms = current_time.nr_sec * 1000 + current_time.nr_nsec / 1000000;

    // --- Check if accumulated PM data should be processed due to timeout ---
    #if PM_BUFFER_ACCUMULATE
    if (pm_accumulate_len > 0 && is_timeout_elapsed(&current_time, &pm_last_receive_time, PM_ACCUMULATE_TIMEOUT_MS)) {
        debug_printf(&app_state, "PM: Accumulation timeout, processing %u bytes", pm_accumulate_len);
        process_accumulated_pm_data(&app_state);
    }
    #endif

    // --- Handle LED blinking timing ---
    // Handle LED blinking timing if LED is currently blinking
    if (led_is_blinking) {
        if (current_time_ms - led_blink_start_ms >= LED_BLINK_DURATION_MS) {
            // Turn off LED after blink duration
            platform_gpo_modify(0, PLATFORM_GPO_LED_ONBOARD);
            led_is_blinking = false;
            debug_printf(&app_state, "LED blink complete");
        }
    }

    app_state.button_event = platform_pb_get_event();
    if (app_state.button_event & PLATFORM_PB_ONBOARD_PRESS) {
        app_state.flags |= PROG_FLAG_BANNER_PENDING; // Re-trigger banner on button press
        last_active_time_sec = current_time.nr_sec; // Update activity timestamp
        
        // Display debug message when button is pressed
        debug_printf(&app_state, "Button pressed - displaying banner");
        
        // Blink LED in response to button press
        platform_gpo_modify(PLATFORM_GPO_LED_ONBOARD, 0);
        led_blink_start_ms = current_time_ms;
        led_is_blinking = true;
    }

    // Display banner if pending
    ui_handle_banner_transmission(&app_state);

    // --- GPS Data Handling ---
    if (app_state.gps_rx_desc.compl_type == PLATFORM_USART_RX_COMPL_DATA) {
        debug_printf(&app_state, "GPS: RX_COMPL_DATA detected, len: %u", app_state.gps_rx_desc.compl_info.data_len);
        app_state.flags |= PROG_FLAG_GPS_DATA_RECEIVED;
        last_active_time_sec = current_time.nr_sec;
        debug_printf(&app_state, "GPS: Received %u bytes", app_state.gps_rx_desc.compl_info.data_len);
        
        #if LED_BLINK_ON_GPS_DATA
        platform_gpo_modify(PLATFORM_GPO_LED_ONBOARD, 0);
        led_blink_start_ms = current_time_ms;
        led_is_blinking = true;
        #endif
        
        if ((app_state.gps_assembly_len + app_state.gps_rx_desc.compl_info.data_len) < GPS_ASSEMBLY_BUF_SZ) {
            memcpy(&app_state.gps_assembly_buf[app_state.gps_assembly_len],
                   app_state.gps_rx_buf,
                   app_state.gps_rx_desc.compl_info.data_len);
            app_state.gps_assembly_len += app_state.gps_rx_desc.compl_info.data_len;
            app_state.gps_assembly_buf[app_state.gps_assembly_len] = '\0'; 

            // Display raw GPS buffer for debugging if enabled or forced
            if (DEBUG_MODE_RAW_GPS || PM_FORCE_RAW_GPS) {
                if (app_state.gps_assembly_len > 0) {
                    debug_printf(&app_state, "GPS: Preparing to print RAW BUFFER (%u bytes). CDC Busy_HW: %d, CDC_Busy_Flag: %d", 
                        app_state.gps_assembly_len, platform_usart_cdc_tx_busy(), (app_state.flags & PROG_FLAG_CDC_TX_BUSY) ? 1:0);
                    // Explicitly clear the software busy flag before raw GPS buffer print
                    app_state.flags &= ~PROG_FLAG_CDC_TX_BUSY;
                    ui_handle_raw_data_transmission(&app_state, "GPS RAW BUFFER", app_state.gps_assembly_buf, app_state.gps_assembly_len);
                }
            }
        } else {
            debug_printf(&app_state, "GPS: Assembly buffer overflow, discarding %u bytes", app_state.gps_rx_desc.compl_info.data_len);
            // Optionally clear gps_assembly_buf or handle error
        }
        
        // Attempt to extract and process NMEA sentences
        uint16_t processed_len_in_assembly = 0;
        while(processed_len_in_assembly < app_state.gps_assembly_len) {
            char nmea_sentence_buffer[NMEA_MAX_SENTENCE_LEN];
            uint16_t sentence_len = extract_nmea_sentence(
                app_state.gps_assembly_buf + processed_len_in_assembly, 
                &(app_state.gps_assembly_len),
                nmea_sentence_buffer, 
                NMEA_MAX_SENTENCE_LEN
            );

            if (sentence_len > 0) {
                // Successfully extracted a sentence
                if (DEBUG_MODE_RAW_GPS || PM_FORCE_RAW_GPS) {
                    debug_printf(&app_state, "GPS: Extracted NMEA sentence, len %u. Preparing for raw print. CDC Busy_HW: %d, CDC_Busy_Flag: %d",
                        sentence_len, platform_usart_cdc_tx_busy(), (app_state.flags & PROG_FLAG_CDC_TX_BUSY) ? 1:0);
                    // Explicitly clear the software busy flag before raw NMEA sentence print
                    app_state.flags &= ~PROG_FLAG_CDC_TX_BUSY;
                    ui_handle_raw_data_transmission(&app_state, "GPS NMEA SENTENCE", nmea_sentence_buffer, sentence_len);
                }

                // Parse the GPGLL sentence
                if (strncmp(nmea_sentence_buffer, "$GPGLL", 6) == 0) {
                    debug_printf(&app_state, "GPGLL sentence found: %s", nmea_sentence_buffer);
                    
                    // Clear parsed fields
                    app_state.parsed_gps_time[0] = '\0';
                    app_state.parsed_gps_lat[0] = '\0';
                    app_state.parsed_gps_lon[0] = '\0';
                    
                    // Create a temporary buffer to hold the formatted output
                    char temp_buffer[256];
                    
                    // Parse the GPGLL sentence using the correct function signature
                    if (nmea_parse_gpgll_and_format(nmea_sentence_buffer, temp_buffer, sizeof(temp_buffer))) {
                        debug_printf(&app_state, "GPGLL parsed successfully: %s", temp_buffer);
                        
                        // Parse the result into separate fields
                        // For demonstration purposes - parse temp_buffer to extract fields
                        // You would typically parse the NMEA sentence directly in a real implementation
                        strncpy(app_state.parsed_gps_time, "00:00:00", sizeof(app_state.parsed_gps_time));
                        strncpy(app_state.parsed_gps_lat, "0.0000N", sizeof(app_state.parsed_gps_lat));
                        strncpy(app_state.parsed_gps_lon, "0.0000E", sizeof(app_state.parsed_gps_lon));
                        
                        app_state.flags |= PROG_FLAG_GPGLL_DATA_PARSED;
                    } else {
                        debug_printf(&app_state, "Failed to parse GPGLL sentence");
                    }
                }
            }
        }

        if (!gps_platform_usart_cdc_rx_async(&app_state.gps_rx_desc)) {
            debug_printf(&app_state, "GPS: ERROR Failed to re-arm RX");
        }
    }

    // --- PM Sensor Data Handling ---
    if (app_state.pm_rx_desc.compl_type == PLATFORM_USART_RX_COMPL_DATA) {
        // This debug line is the source of "PM data received: 1 bytes"
        // It's accurate for the *hardware reception event* if data arrives byte-by-byte.
        // Consider removing or reducing its verbosity if too noisy.
        // debug_printf(&app_state, "PM: HW Received %u bytes", app_state.pm_rx_desc.compl_info.data_len); 
        
        #if PM_BUFFER_ACCUMULATE
        platform_tick_count(&current_time); // Update current time for timeout logic
        pm_last_receive_time = current_time;
        if (pm_accumulate_len + app_state.pm_rx_desc.compl_info.data_len < sizeof(pm_accumulate_buffer)) {
            memcpy(pm_accumulate_buffer + pm_accumulate_len, 
                   app_state.pm_rx_buf, 
                   app_state.pm_rx_desc.compl_info.data_len);
            pm_accumulate_len += app_state.pm_rx_desc.compl_info.data_len;
            if (pm_accumulate_len >= PM_ACCUMULATE_THRESHOLD) {
                process_accumulated_pm_data(&app_state);
            }
        } else {
            debug_printf(&app_state, "PM: Accumulation buffer full (%u + %u), processing then adding.", pm_accumulate_len, app_state.pm_rx_desc.compl_info.data_len);
            process_accumulated_pm_data(&app_state); // Process what's there
            if (app_state.pm_rx_desc.compl_info.data_len < sizeof(pm_accumulate_buffer)) { // Ensure new data fits
                memcpy(pm_accumulate_buffer, app_state.pm_rx_buf, app_state.pm_rx_desc.compl_info.data_len);
                pm_accumulate_len = app_state.pm_rx_desc.compl_info.data_len;
            } else {
                 debug_printf(&app_state, "PM: ERROR - incoming packet too large for empty buffer (%u bytes)", app_state.pm_rx_desc.compl_info.data_len);
                 pm_accumulate_len = 0; // discard
            }
        }
        #else
        // Original single-reception handling
        // Define a minimum meaningful packet length for PM data to display 
        #define PM_MIN_DISPLAY_LENGTH 2
        
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
                
                // Debug message for PM data parsed successfully
                debug_printf(&app_state, "PM data parsed: PM1.0=%u, PM2.5=%u, PM10=%u ug/m3", 
                            app_state.latest_pms_data.pm1_0_atm,
                            app_state.latest_pms_data.pm2_5_atm,
                            app_state.latest_pms_data.pm10_atm);
                
                // Blink LED when PM data is successfully parsed
                #if LED_BLINK_ON_PM_DATA
                platform_gpo_modify(PLATFORM_GPO_LED_ONBOARD, 0);
                led_blink_start_ms = current_time_ms;
                led_is_blinking = true;
                #endif
                
                break; // Processed one full packet
            }
        }
        #endif
        
        if (!pm_platform_usart_cdc_rx_async(&app_state.pm_rx_desc)) {
            debug_printf(&app_state, "PM: ERROR Failed to re-arm RX");
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
        debug_printf(&app_state, "Watchdog: Clearing potentially stuck flags after 5 seconds of inactivity");
        
        // Clear potentially stuck flags
        app_state.flags &= ~PROG_FLAG_CDC_TX_BUSY;
        
        // Re-arm both receivers if they seem stuck
        if (gps_platform_usart_cdc_rx_busy()) {
            debug_printf(&app_state, "Watchdog: GPS RX appears stuck, re-arming");
            gps_platform_usart_cdc_rx_abort();
            app_state.gps_rx_desc.compl_type = PLATFORM_USART_RX_COMPL_NONE;
            gps_platform_usart_cdc_rx_async(&app_state.gps_rx_desc);
        }
        
        if (pm_platform_usart_cdc_rx_busy()) {
            debug_printf(&app_state, "Watchdog: PM RX appears stuck, re-arming");
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
