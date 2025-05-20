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

// Define for UART wait timeout
#define UART_WAIT_TIMEOUT_COUNT 30000

// Global application state variable
static prog_state_t app_state;

// Configuration constants (can be moved to main.h or a config.h)
#define DEBUG_MODE_RAW_GPS                  1 // Always enable raw GPS data printing
#define DEBUG_MODE_RAW_PM                   1 // Always enable raw PM data printing
#define PM_FORCE_RAW_GPS                    0 // Disable forcing raw GPS for PM testing
#define DEBUG_LEVEL_VERBOSE                 0 // Keep verbose debug off by default

// Add LED blinking define constants
#define LED_BLINK_ON_GPS_DATA   1 // Blink LED when GPS data is received
#define LED_BLINK_ON_PM_DATA    1 // Blink LED when PM data is received and parsed
#define LED_BLINK_DURATION_MS   50 // How long to blink the LED in milliseconds

// Define constants for PM data accumulation
#define PM_BUFFER_ACCUMULATE    1 // Set to 1 to accumulate PM data into a buffer
#define PM_ACCUMULATE_THRESHOLD 32 // Increased to typical PMS5003 packet size

// PM data accumulation buffer
static uint8_t pm_accumulate_buffer[PM_RX_BUF_SZ * 2]; // Double size buffer to allow accumulation
static uint16_t pm_accumulate_len = 0;
static platform_timespec_t pm_last_receive_time = {0, 0};
static const uint32_t PM_ACCUMULATE_TIMEOUT_MS = 300; // Slightly increased timeout for accumulated buffer

// LED blinking control variables - moved to module level to be accessible from all functions
static uint32_t led_blink_start_ms = 0;
static bool led_is_blinking = false;

// Forward declarations
void debug_printf(prog_state_t *ps, const char *fmt, ...);
void ui_display_combined_data(prog_state_t *ps);

// Function to print raw GPS data by calling the modified debug_printf for each line
static void debug_print_gps_raw_data(prog_state_t *ps, const char *raw_data_buffer, uint16_t raw_data_len) {
    if (raw_data_len == 0) {
        #if DEBUG_LEVEL_VERBOSE
        debug_printf(ps, "[DEBUG] GPS Raw Data: (empty)\r\n");
        #endif
        return;
    }

    // Print the label first
    debug_printf(ps, "GPS Raw Data:"); // debug_printf will add [DEBUG] and \r\n

    const char *ptr = raw_data_buffer;
    const char *end = raw_data_buffer + raw_data_len;
    char line_buffer[NMEA_MAX_SENTENCE_LEN + 1]; // Buffer for a single line of GPS data

    while (ptr < end) {
        const char *line_end = ptr;
        // Find the end of the current line (look for \n or \r)
        while (line_end < end && *line_end != '\n' && *line_end != '\r') {
            line_end++;
        }

        uint16_t line_len = line_end - ptr;
        if (line_len > 0) {
            if (line_len > NMEA_MAX_SENTENCE_LEN) {
                line_len = NMEA_MAX_SENTENCE_LEN; // Truncate if too long for buffer
            }
            memcpy(line_buffer, ptr, line_len);
            line_buffer[line_len] = '\0';
            // Call the robust debug_printf for each individual line of GPS data
            debug_printf(ps, "%s", line_buffer);
        }

        // Move pointer past the line ending characters for the next iteration
        ptr = line_end;
        while (ptr < end && (*ptr == '\n' || *ptr == '\r')) {
            ptr++;
        }
    }
}

// Add helper function to print hexdump for debugging by calling the modified debug_printf for each line
static void debug_print_hex(prog_state_t *ps, const uint8_t *data, uint16_t len) {
    // Print the header for the hexdump
    if (len == 0) {
        debug_printf(ps, "PM: the hexdump (0 bytes):");
        return;
    }
    debug_printf(ps, "PM: the hexdump (%u bytes):", len);

    char hex_line_buffer[80]; // Suitable buffer for one line of hex (e.g., 16 bytes * 3 chars + null)
    int current_line_char_count;

    for (uint16_t i = 0; i < len; i += 16) { // Process data in chunks of 16 bytes for each line
        current_line_char_count = 0;
        for (uint16_t j = 0; j < 16 && (i + j) < len; ++j) {
            current_line_char_count += snprintf(hex_line_buffer + current_line_char_count, 
                                              sizeof(hex_line_buffer) - current_line_char_count, 
                                              "%02X ", 
                                              data[i + j]);
        }
        if (current_line_char_count > 0) {
            // Remove trailing space if one exists from the last %02X 
            if (hex_line_buffer[current_line_char_count - 1] == ' ') {
                hex_line_buffer[current_line_char_count - 1] = '\0';
            }
            // Call the robust debug_printf for each formatted line of hex characters
            debug_printf(ps, "%s", hex_line_buffer);
        }
    }
}

// New function: direct_printf
void direct_printf(prog_state_t *ps, const char *fmt, ...) {
    uint32_t entry_wait_timeout = UART_WAIT_TIMEOUT_COUNT; 
    while(((ps->flags & PROG_FLAG_CDC_TX_BUSY) || platform_usart_cdc_tx_busy()) && entry_wait_timeout-- > 0) {
        platform_do_loop_one();
    }
    if ((ps->flags & PROG_FLAG_CDC_TX_BUSY) || platform_usart_cdc_tx_busy()) {
        return; 
    }
    
    // MODIFICATION: Use a static buffer for vsnprintf, similar to debug_printf
    static char direct_local_format_buf[CDC_TX_BUF_SZ]; 
    // END MODIFICATION

    va_list args;
    va_start(args, fmt);
    // Use the new static buffer here
    int len = vsnprintf(direct_local_format_buf, sizeof(direct_local_format_buf), fmt, args);
    va_end(args);
    
    if (len <= 0) { 
        return; 
    }
    // Ensure null termination if vsnprintf filled the buffer exactly (though vsnprintf should handle this)
    // or if it truncated.
    if (len >= sizeof(direct_local_format_buf)) { 
        len = sizeof(direct_local_format_buf) - 1; 
        direct_local_format_buf[len] = '\0'; // Explicitly null-terminate
    }

    // Buffer for the final message, ensuring space for \r\n
    char final_msg_buf[CDC_TX_BUF_SZ + 3]; // +2 for \r\n, +1 for null. Max content is CDC_TX_BUF_SZ.
                                           // So, CDC_TX_BUF_SZ (content) + 2 (\r\n) + 1 (null)
                                           // If direct_local_format_buf is CDC_TX_BUF_SZ, it already includes null.
                                           // So, final_msg_buf needs to be at least direct_local_format_buf's size + 2 for \r\n if not present.
                                           // Let's use CDC_TX_BUF_SZ as the base for content, then add.
                                           // Max possible length of direct_local_format_buf is sizeof(direct_local_format_buf)-1.
                                           // Max length of final_msg_buf will be (sizeof(direct_local_format_buf)-1) + 2.
                                           // So, sizeof(direct_local_format_buf) + 1 is sufficient.
                                           // Given CDC_TX_BUF_SZ is 256, a 256+2 buffer is fine.

    bool ends_with_crlf = (len >= 2 && direct_local_format_buf[len-2] == '\r' && direct_local_format_buf[len-1] == '\n');
    bool ends_with_lf = (!ends_with_crlf && len >=1 && direct_local_format_buf[len-1] == '\n');

    int final_len;
    if (ends_with_crlf) {
        // Content already has \r\n
        strncpy(final_msg_buf, direct_local_format_buf, sizeof(final_msg_buf) -1 );
        final_msg_buf[sizeof(final_msg_buf)-1] = '\0'; // ensure null termination
        final_len = len; 
    } else if (ends_with_lf) {
        // Content has \n, replace with \r\n
        final_len = snprintf(final_msg_buf, sizeof(final_msg_buf), "%.*s\r\n", len-1, direct_local_format_buf);
    } else {
        // Content has no newline, add \r\n
        final_len = snprintf(final_msg_buf, sizeof(final_msg_buf), "%s\r\n", direct_local_format_buf);
    }
    
    // Check final_len against the actual shared hardware buffer ps->cdc_tx_buf
    if (final_len <= 0 || (size_t)final_len >= sizeof(ps->cdc_tx_buf)) { 
        return; 
    }

    memcpy(ps->cdc_tx_buf, final_msg_buf, final_len + 1); // final_len is from snprintf/strlen, already accounts for content without null
                                                        // or includes null if strncpy was used carefully.
                                                        // memcpy final_len+1 to include null.
    
    ps->cdc_tx_desc[0].buf = ps->cdc_tx_buf;
    ps->cdc_tx_desc[0].len = final_len; // Length for USART should not include null terminator
    
    ps->flags |= PROG_FLAG_CDC_TX_BUSY; 

    if (platform_usart_cdc_tx_async(&ps->cdc_tx_desc[0], 1)) {
        uint32_t hardware_wait_timeout = UART_WAIT_TIMEOUT_COUNT; 
        while(platform_usart_cdc_tx_busy() && hardware_wait_timeout-- > 0) {
            platform_do_loop_one();
        }
        ps->flags &= ~PROG_FLAG_CDC_TX_BUSY;
    } else {
        ps->flags &= ~PROG_FLAG_CDC_TX_BUSY;
    }
}

/**
 * @brief Basic debug print function - MODIFIED FOR SAFER CDC ACCESS AND SERIALIZATION.
 * Sends formatted string to CDC terminal if not busy.
 * MUST be passed the app_state pointer.
 */
void debug_printf(prog_state_t *ps, const char *fmt, ...) {
    // MODIFICATION START: Conditionalize on ps->is_debug
    if (!ps->is_debug) {
        return; // Do not print if global debug flag is off
    }
    // MODIFICATION END

    uint32_t entry_wait_timeout = UART_WAIT_TIMEOUT_COUNT; 
    while(((ps->flags & PROG_FLAG_CDC_TX_BUSY) || platform_usart_cdc_tx_busy()) && entry_wait_timeout-- > 0) {
        platform_do_loop_one();
    }
    if ((ps->flags & PROG_FLAG_CDC_TX_BUSY) || platform_usart_cdc_tx_busy()) {
        return; 
    }
    
    static char local_debug_format_buf[256]; 
    char final_debug_msg_buf[280]; 

    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(local_debug_format_buf, sizeof(local_debug_format_buf), fmt, args);
    va_end(args);
    
    if (len <= 0 || len >= sizeof(local_debug_format_buf)) {
        return; 
    }

    bool ends_with_crlf = (len >= 2 && local_debug_format_buf[len-2] == '\r' && local_debug_format_buf[len-1] == '\n');
    bool ends_with_lf = (!ends_with_crlf && len >=1 && local_debug_format_buf[len-1] == '\n');

    if (ends_with_crlf) {
         snprintf(final_debug_msg_buf, sizeof(final_debug_msg_buf), "[DEBUG] %s", local_debug_format_buf);
    } else if (ends_with_lf) {
        // local_debug_format_buf[len-1] = '\0'; // This was an error, should not modify buffer passed to %.*s
        snprintf(final_debug_msg_buf, sizeof(final_debug_msg_buf), "[DEBUG] %.*s\r\n", len-1, local_debug_format_buf);
    } else {
        snprintf(final_debug_msg_buf, sizeof(final_debug_msg_buf), "[DEBUG] %s\r\n", local_debug_format_buf);
    }
    len = strlen(final_debug_msg_buf); 
    
    if (len == 0 || (size_t)len >= sizeof(ps->cdc_tx_buf)) { 
        return; 
    }

    memcpy(ps->cdc_tx_buf, final_debug_msg_buf, len + 1); 
    
    ps->cdc_tx_desc[0].buf = ps->cdc_tx_buf;
    ps->cdc_tx_desc[0].len = len;
    
    ps->flags |= PROG_FLAG_CDC_TX_BUSY; 

    if (platform_usart_cdc_tx_async(&ps->cdc_tx_desc[0], 1)) {
        uint32_t hardware_wait_timeout = UART_WAIT_TIMEOUT_COUNT; 
        while(platform_usart_cdc_tx_busy() && hardware_wait_timeout-- > 0) {
            platform_do_loop_one();
        }
        ps->flags &= ~PROG_FLAG_CDC_TX_BUSY;
    } else {
        ps->flags &= ~PROG_FLAG_CDC_TX_BUSY;
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
        #if DEBUG_LEVEL_VERBOSE
        debug_printf(ps, "[DEBUG] PM: No accumulated data to process.\r\n");
        #endif
        return;
    }

    #if DEBUG_LEVEL_VERBOSE 
    debug_printf(ps, "[DEBUG] PM: Processing accumulated %u bytes\r\n", pm_accumulate_len);
    #endif

    // This debug_print_hex IS conditional on ps->is_debug because debug_printf is.
    // And DEBUG_MODE_RAW_PM controls if this specific hexdump is generated.
    if (DEBUG_MODE_RAW_PM) { 
        debug_print_hex(ps, pm_accumulate_buffer, pm_accumulate_len);
    }
    
    bool valid_pm_header = false;
    if (pm_accumulate_len >= 2 && pm_accumulate_buffer[0] == 0x42 && pm_accumulate_buffer[1] == 0x4D) {
        valid_pm_header = true;
        #if DEBUG_LEVEL_VERBOSE
        debug_printf(ps, "PM: Valid header 0x424D found.");
        #endif 
    }

    // MODIFICATION START: Make PM RAW HEX output conditional on ps->is_debug as well
    // if (DEBUG_MODE_RAW_PM) {  // Old condition
    if (ps->is_debug && DEBUG_MODE_RAW_PM) { // New condition
    // MODIFICATION END
        #if DEBUG_LEVEL_VERBOSE
        debug_printf(ps, "PM: Preparing to print RAW HEX (%u bytes). CDC Busy_HW: %d, CDC_Busy_Flag: %d", 
            pm_accumulate_len, platform_usart_cdc_tx_busy(), (ps->flags & PROG_FLAG_CDC_TX_BUSY) ? 1:0);
        #endif 
        ps->flags &= ~PROG_FLAG_CDC_TX_BUSY; 
        ui_handle_raw_data_transmission(ps, "PM RAW HEX", (const char*)pm_accumulate_buffer, pm_accumulate_len);
    }
    
    for (uint16_t i = 0; i < pm_accumulate_len; ++i) {
        pms_parser_status_t status = pms_parser_feed_byte(ps, 
                                                         &ps->pms_parser_state, 
                                                         pm_accumulate_buffer[i],
                                                         &ps->latest_pms_data);
        if (status == PMS_PARSER_OK) {
            ps->flags |= PROG_FLAG_PM_DATA_PARSED;
            // This debug_printf is already conditional on ps->is_debug
            debug_printf(ps, "PM: Parsed OK: PM1.0=%u, PM2.5=%u, PM10=%u", 
                        ps->latest_pms_data.pm1_0_atm,
                        ps->latest_pms_data.pm2_5_atm,
                        ps->latest_pms_data.pm10_atm);
            #if LED_BLINK_ON_PM_DATA
            platform_gpo_modify(PLATFORM_GPO_LED_ONBOARD, 0);
            platform_timespec_t current_time_blink; // Use a different name
            platform_tick_count(&current_time_blink);
            uint32_t current_time_ms_blink = current_time_blink.nr_sec * 1000 + current_time_blink.nr_nsec / 1000000;
            led_blink_start_ms = current_time_ms_blink;
            led_is_blinking = true;
            #endif
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
    snprintf(app_state.formatted_gpggl_string, 
             sizeof(app_state.formatted_gpggl_string), 
             "--:--:-- | Lat: Waiting for data..., - | Long: Waiting for data..., -");
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
    app_state.is_debug = false;
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
    // EXPERIMENTAL: Add more calls to platform_do_loop_one() to ensure
    // sufficient event processing if the loop becomes very fast without debug prints.
    // This is to test if RX was starving due to too few calls when debug_printf is off.
    // This is not an ideal long-term solution but a diagnostic step.
    platform_do_loop_one();
    platform_do_loop_one();
    // END EXPERIMENTAL
    // Get current time for watchdog and other timing
    platform_tick_count(&current_time);
    uint32_t current_time_ms = current_time.nr_sec * 1000 + current_time.nr_nsec / 1000000;

    // --- Check if accumulated PM data should be processed due to timeout --- (TEMPORARILY DISABLED FOR GPS DEBUGGING)
    /*
    #if PM_BUFFER_ACCUMULATE
    if (pm_accumulate_len > 0 && is_timeout_elapsed(&current_time, &pm_last_receive_time, PM_ACCUMULATE_TIMEOUT_MS)) {
        debug_printf(&app_state, "PM: Accumulation timeout, processing %u bytes", pm_accumulate_len);
        process_accumulated_pm_data(&app_state);
    }
    #endif
    */

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
    // New diagnostic print to observe GPS RX descriptor state every loop
    static uint32_t gps_loop_check_counter = 0;
    if (++gps_loop_check_counter >= 1000) { // Print every 1000th iteration
        debug_printf(&app_state, "GPS Loop Check (throttled): compl_type=%d, hw_len=%u, assembly_len=%u",
                     app_state.gps_rx_desc.compl_type, app_state.gps_rx_desc.compl_info.data_len, app_state.gps_assembly_len);
        gps_loop_check_counter = 0; // Reset counter
    }

    if (app_state.gps_rx_desc.compl_type == PLATFORM_USART_RX_COMPL_DATA) {
        // Ensure this primary debug message is active and informative
        debug_printf(&app_state, "GPS: RX_COMPL_DATA event. Hardware len: %u.", 
                     app_state.gps_rx_desc.compl_info.data_len);
        
        app_state.flags |= PROG_FLAG_GPS_DATA_RECEIVED;
        last_active_time_sec = current_time.nr_sec;
        
        #if LED_BLINK_ON_GPS_DATA
        platform_gpo_modify(PLATFORM_GPO_LED_ONBOARD, 0);
        led_blink_start_ms = current_time_ms;
        led_is_blinking = true;
        #endif

        // SIMPLIFIED GPS DATA HANDLING FOR DEBUGGING:
        debug_printf(&app_state, "GPS RAW DATA DETECTED - PRINTING DIRECT BUFFER CONTENTS");
        if (app_state.gps_rx_desc.compl_info.data_len > 0) {
            debug_print_gps_raw_data(&app_state, (const char*)app_state.gps_rx_buf, app_state.gps_rx_desc.compl_info.data_len);
        } else {
            debug_printf(&app_state, "GPS: RX_COMPL_DATA event, but hardware len is 0.");
        }

        // Re-arm asynchronous reception for GPS
        app_state.gps_rx_desc.compl_type = PLATFORM_USART_RX_COMPL_NONE;
        // Explicitly clear compl_info as well
        memset((void*)&app_state.gps_rx_desc.compl_info, 0, sizeof(app_state.gps_rx_desc.compl_info));
        app_state.gps_rx_desc.buf = app_state.gps_rx_buf; // Ensure buffer pointer is correct
        app_state.gps_rx_desc.max_len = GPS_RX_BUF_SZ;    // Ensure max length is correct
        if (!gps_platform_usart_cdc_rx_async(&app_state.gps_rx_desc)) {
            debug_printf(&app_state, "GPS: ERROR Failed to re-arm RX");
        }
    }

    // --- PM Sensor Data Accumulation Timeout Logic ---

    // The variable 'current_time_ms' is already calculated at the beginning of prog_loop_one.

    // We use 'pm_last_receive_time' (which should be updated when PM data is received)

    // and 'PM_ACCUMULATE_TIMEOUT_MS'.

    bool pm_timeout_for_processing = false;

    if (pm_accumulate_len > 0) { // Only check timeout if there's data to process

        platform_timespec_t current_pm_timeout_check_time;

        platform_tick_count(&current_pm_timeout_check_time);

        if (is_timeout_elapsed(&current_pm_timeout_check_time, &pm_last_receive_time, PM_ACCUMULATE_TIMEOUT_MS)) {

            pm_timeout_for_processing = true;

            #if DEBUG_LEVEL_VERBOSE

            debug_printf(&app_state, "[DEBUG] PM: Accumulation timeout. Processing %u bytes.\r\n", pm_accumulate_len);

            #endif

        }

    }



    // --- PM Sensor Data Handling ---

    if (app_state.pm_rx_desc.compl_type == PLATFORM_USART_RX_COMPL_DATA) {

        uint16_t RcvdBytes = app_state.pm_rx_desc.compl_info.data_len;



        if (RcvdBytes > 0) {

            platform_tick_count(&pm_last_receive_time); // Update last receive time on new data



            // Append to accumulation buffer

            if ((pm_accumulate_len + RcvdBytes) <= sizeof(pm_accumulate_buffer)) {

                memcpy(pm_accumulate_buffer + pm_accumulate_len, app_state.pm_rx_buf, RcvdBytes);

                pm_accumulate_len += RcvdBytes;

            } else {

                #if DEBUG_LEVEL_VERBOSE

                debug_printf(&app_state, "[DEBUG] PM: Accumulation buffer overflow. Discarding %u, had %u\r\n", RcvdBytes, pm_accumulate_len);

                #endif

                // Overflow: clear previous, copy new. This might lose context but prevents full lockup.

                memset(pm_accumulate_buffer, 0, sizeof(pm_accumulate_buffer));

                memcpy(pm_accumulate_buffer, app_state.pm_rx_buf, (RcvdBytes > sizeof(pm_accumulate_buffer)) ? sizeof(pm_accumulate_buffer) : RcvdBytes);

                pm_accumulate_len = (RcvdBytes > sizeof(pm_accumulate_buffer)) ? sizeof(pm_accumulate_buffer) : RcvdBytes;

            }

        }



        // Reset and re-arm PM RX for next packet, regardless of whether we process now or later

        memset((void*)&app_state.pm_rx_desc.compl_info, 0, sizeof(app_state.pm_rx_desc.compl_info));

        app_state.pm_rx_desc.buf = app_state.pm_rx_buf;

        app_state.pm_rx_desc.max_len = PM_RX_BUF_SZ;

        app_state.pm_rx_desc.compl_type = PLATFORM_USART_RX_COMPL_NONE;

        if (!pm_platform_usart_cdc_rx_busy()) { 

             pm_platform_usart_cdc_rx_async(&app_state.pm_rx_desc);

        }

        // Note: last_pm_activity_time was removed; pm_last_receive_time is used for timeout logic.

    }



    // Process accumulated PM data if threshold is met OR if a timeout occurred with some data

    if (pm_accumulate_len >= PM_ACCUMULATE_THRESHOLD || (pm_timeout_for_processing && pm_accumulate_len > 0)) {

        if (DEBUG_MODE_RAW_PM) { 

            process_accumulated_pm_data(&app_state); // This function now handles its own hexdump

        }

        // Reset accumulation buffer AFTER processing

        pm_accumulate_len = 0;

        memset(pm_accumulate_buffer, 0, sizeof(pm_accumulate_buffer));

    }



    // --- GPS Data Handling ---
    if (++gps_loop_check_counter >= 1000) { // Print every 1000 loops
        #if DEBUG_LEVEL_VERBOSE 
        // This verbose check can be enabled if deep debugging of GPS RX is needed again.
        debug_printf(&app_state, "GPS Loop Check (throttled): compl_type=%d, hw_len=%u, assembly_len=%u\r\n",
                     app_state.gps_rx_desc.compl_type,
                     app_state.gps_rx_desc.compl_info.data_len,
                     app_state.gps_assembly_len);
        #endif
        gps_loop_check_counter = 0;
    }

    if (app_state.gps_rx_desc.compl_type == PLATFORM_USART_RX_COMPL_DATA) {
        uint16_t hw_len = app_state.gps_rx_desc.compl_info.data_len;
        // No longer needed: debug_printf(&app_state, "GPS: RX_COMPL_DATA event. Hardware len: %u.\r\n", hw_len);

        if (hw_len > 0) {
            // No longer needed: debug_printf(&app_state, "GPS RAW DATA DETECTED - PRINTING DIRECT BUFFER CONTENTS\r\n");
            if (DEBUG_MODE_RAW_GPS) {
                 debug_print_gps_raw_data(&app_state, (const char*)app_state.gps_rx_buf, hw_len);
            }

            // Append to assembly buffer
            if (app_state.gps_assembly_len + hw_len < GPS_ASSEMBLY_BUF_SZ) {
                memcpy(app_state.gps_assembly_buf + app_state.gps_assembly_len, app_state.gps_rx_buf, hw_len);
                app_state.gps_assembly_len += hw_len;
                app_state.gps_assembly_buf[app_state.gps_assembly_len] = '\0'; // Null-terminate
            } else {
                #if DEBUG_LEVEL_VERBOSE
                debug_printf(&app_state, "GPS Assembly Overflow. Had: %u, Got: %u. Clearing.\r\n", app_state.gps_assembly_len, hw_len);
                #endif
                // Overflow: clear assembly, copy current hw_len (maybe it's a new sentence)
                memset(app_state.gps_assembly_buf, 0, GPS_ASSEMBLY_BUF_SZ);
                memcpy(app_state.gps_assembly_buf, app_state.gps_rx_buf, (hw_len < GPS_ASSEMBLY_BUF_SZ) ? hw_len : GPS_ASSEMBLY_BUF_SZ -1);
                app_state.gps_assembly_len = (hw_len < GPS_ASSEMBLY_BUF_SZ) ? hw_len : GPS_ASSEMBLY_BUF_SZ -1;
                app_state.gps_assembly_buf[app_state.gps_assembly_len] = '\0';
            }

            // Process assembled data for NMEA sentences
            char *sentence_start = app_state.gps_assembly_buf;
            char *sentence_end;
            uint16_t processed_len_total = 0;

            while ((sentence_end = strstr(sentence_start, "\r\n")) != NULL) {
                uint16_t sentence_len = (sentence_end - sentence_start) + 2; // Include <CR><LF>

                // Temporarily null-terminate for parsing functions
                char original_char_after_cr = *(sentence_end);
                char original_char_after_lf = *(sentence_end+1);
                *sentence_end = '\0'; 
                // *(sentence_end + 1) = '\0'; // Not needed if strstr stops at first 

                if (strncmp(sentence_start, "$GPGLL", 6) == 0) {
                    if (nmea_parse_gpgll_and_format(sentence_start, 
                                                 app_state.formatted_gpggl_string, 
                                                 sizeof(app_state.formatted_gpggl_string))) {
                        app_state.flags |= PROG_FLAG_GPGLL_DATA_PARSED;
                        #if DEBUG_LEVEL_VERBOSE
                        debug_printf(&app_state, "GPGLL Parsed & Formatted OK: %s\r\n", app_state.formatted_gpggl_string);
                        #endif
                    } else {
                        #if DEBUG_LEVEL_VERBOSE
                        debug_printf(&app_state, "GPGLL Parse/Format Failed for: %s\r\n", sentence_start);
                        #endif
                        // Clear the formatted string buffer on failure to avoid displaying stale data
                        app_state.formatted_gpggl_string[0] = '\0';
                    }
                }
                // Add other NMEA sentence parsers here if needed (e.g., GPRMC, GPGGA)

                // Restore sentence_end characters if they were part of a larger buffer
                *sentence_end = original_char_after_cr;
                // *(sentence_end + 1) = original_char_after_lf;


                processed_len_total += sentence_len;
                sentence_start = sentence_end + 2; // Move past <CR><LF>
            }

            // Shift remaining data in assembly buffer
            if (processed_len_total > 0 && processed_len_total <= app_state.gps_assembly_len) {
                memmove(app_state.gps_assembly_buf, app_state.gps_assembly_buf + processed_len_total, app_state.gps_assembly_len - processed_len_total);
                app_state.gps_assembly_len -= processed_len_total;
                app_state.gps_assembly_buf[app_state.gps_assembly_len] = '\0'; // Null-terminate new end
            } else if (processed_len_total > app_state.gps_assembly_len) {
                // Should not happen, indicates error in logic
                app_state.gps_assembly_len = 0;
                app_state.gps_assembly_buf[0] = '\0';
            }
        }

        // Reset and re-arm GPS RX
        memset((void*)&app_state.gps_rx_desc.compl_info, 0, sizeof(app_state.gps_rx_desc.compl_info));
        app_state.gps_rx_desc.buf = app_state.gps_rx_buf;
        app_state.gps_rx_desc.max_len = GPS_RX_BUF_SZ;
        app_state.gps_rx_desc.compl_type = PLATFORM_USART_RX_COMPL_NONE;
        if (!gps_platform_usart_cdc_rx_busy()) { // Check if already busy
            gps_platform_usart_cdc_rx_async(&app_state.gps_rx_desc);
        }
    }

    // --- Combined Data Display Logic ---
    // Example: Display data every 1 second if both GPS and PM data are ready
    if (app_state.flags & PROG_FLAG_PM_DATA_PARSED) { // Trigger if PM data is ready
        if ((current_time_ms - app_state.last_display_timestamp) >= app_state.display_interval_ms) {
            ui_display_combined_data(&app_state); // This function handles printing based on available data

            // Clear the PM flag as it has been processed for display.
            app_state.flags &= ~PROG_FLAG_PM_DATA_PARSED;

            // If GPGLL data was also parsed and displayed, clear its flag too.
            // ui_display_combined_data uses PROG_FLAG_GPGLL_DATA_PARSED to determine what to print for GPS.
            // If it was set, it means the (potentially valid or "N/A") GPS data was included in the display.
            if (app_state.flags & PROG_FLAG_GPGLL_DATA_PARSED) {
                 app_state.flags &= ~PROG_FLAG_GPGLL_DATA_PARSED;
            }
            
            app_state.last_display_timestamp = current_time_ms;
        }
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
            memset((void*)&app_state.gps_rx_desc.compl_info, 0, sizeof(app_state.gps_rx_desc.compl_info));
            app_state.gps_rx_desc.buf = app_state.gps_rx_buf;
            app_state.gps_rx_desc.max_len = GPS_RX_BUF_SZ;
            if (!gps_platform_usart_cdc_rx_async(&app_state.gps_rx_desc)) {
                debug_printf(&app_state, "Watchdog: ERROR Failed to re-arm GPS RX after abort.");
            } else {
                debug_printf(&app_state, "Watchdog: GPS RX re-armed successfully after abort.");
            }
        }
        
        if (pm_platform_usart_cdc_rx_busy()) {
            debug_printf(&app_state, "Watchdog: PM RX appears stuck, re-arming");
            pm_platform_usart_cdc_rx_abort();
            app_state.pm_rx_desc.compl_type = PLATFORM_USART_RX_COMPL_NONE;
            memset((void*)&app_state.pm_rx_desc.compl_info, 0, sizeof(app_state.pm_rx_desc.compl_info));
            app_state.pm_rx_desc.buf = app_state.pm_rx_buf;
            app_state.pm_rx_desc.max_len = PM_RX_BUF_SZ;
            if (!pm_platform_usart_cdc_rx_async(&app_state.pm_rx_desc)) {
                debug_printf(&app_state, "Watchdog: ERROR Failed to re-arm PM RX after abort.");
            } else {
                debug_printf(&app_state, "Watchdog: PM RX re-armed successfully after abort.");
            }
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

// File: src/main.c
// Function: ui_display_combined_data
// Showing the modification to the provided block:

void ui_display_combined_data(prog_state_t *ps) {
    // Ensure UART is free before attempting to print a new set of data
    uint32_t entry_wait_timeout = UART_WAIT_TIMEOUT_COUNT; // Timeout for waiting
    while(((ps->flags & PROG_FLAG_CDC_TX_BUSY) || platform_usart_cdc_tx_busy()) && entry_wait_timeout-- > 0) {
        platform_do_loop_one(); // Allow platform to tick
    }
    if (entry_wait_timeout == 0 && ((ps->flags & PROG_FLAG_CDC_TX_BUSY) || platform_usart_cdc_tx_busy())) {
        // Still busy, skip this display cycle
        return;
    }
    
    char temp_format_buf[CDC_TX_BUF_SZ]; 

    // MODIFICATION START:
    // The if/else block is removed.
    // We now always use ps->formatted_gpggl_string.
    // Its content is managed by nmea_parse_gpgll_and_format and initialized in prog_setup.
    snprintf(temp_format_buf, sizeof(temp_format_buf),
             "%sGPS: %s%s | %sPM: PM1.0: %u, PM2.5: %u, PM10: %u%s",
             ANSI_MAGENTA, 
             ps->formatted_gpggl_string, // Always use this string for the GPS part
             ANSI_RESET, 
             ANSI_YELLOW,  
             ps->latest_pms_data.pm1_0_atm, 
             ps->latest_pms_data.pm2_5_atm, 
             ps->latest_pms_data.pm10_atm,
             ANSI_RESET); 
    // MODIFICATION END

    direct_printf(ps, "%s", temp_format_buf);

    // Flag clearing is handled by the caller in prog_loop_one
    // The line `ps->flags &= ~(PROG_FLAG_GPGLL_DATA_PARSED | PROG_FLAG_PM_DATA_PARSED);`
    // was correctly removed from here in previous steps, and is handled in prog_loop_one.
}