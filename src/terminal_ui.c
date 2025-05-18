/**
 * @file terminal_ui.c
 * @brief Implementation of terminal user interface for combined GPS and PM sensor project.
 * @date [Current Date]
 */

#include "../inc/terminal_ui.h"
#include "../inc/main.h"
#include "../inc/platform.h"
#include <stdio.h>
#include <string.h>

// ANSI Escape Sequences
static const char ANSI_RESET[] = "\033[0m";
static const char ANSI_CLEAR_SCREEN[] = "\033[2J";
static const char ANSI_CURSOR_HOME[] = "\033[1;1H";
static const char ANSI_CLEAR_LINE[] = "\033[K";
static const char ANSI_BOLD[] = "\033[1m";
static const char ANSI_GREEN[] = "\033[32m";
static const char ANSI_CYAN[] = "\033[36m";

// Banner text
static const char banner_text[] =
    "\033[0m"       // Reset terminal formatting
    "\033[2J"       // Clear the entire screen
    "\033[1;1H"     // Move cursor to top-left
    "+--------------------------------------------------------------------+\r\n"
    "| EEE 192: Electrical and Electronics Engineering Laboratory VI      |\r\n"
    "|          Academic Year 2024-2025, Semester 2                       |\r\n"
    "|                                                                    |\r\n"
    "| Combined Project: GPS Module and PM Sensor                         |\r\n"
    "|                                                                    |\r\n"
    "| Authors: De Villa, Estrada, & Ramos (EEE 192 2S)                   |\r\n"
    "| Date:    2025                                                      |\r\n"
    "+--------------------------------------------------------------------+\r\n"
    "\r\n";

/**
 * @brief Handles the transmission of the application banner to the terminal.
 *
 * @param ps Pointer to the program state structure.
 */
void ui_handle_banner_transmission(struct prog_state_type *ps) {
    // Check if the banner display is pending and CDC TX is available
    if (!(ps->flags & PROG_FLAG_BANNER_PENDING)) {
        return;
    }
    
    if (platform_usart_cdc_tx_busy()) {
        return;
    }
    
    if (ps->flags & PROG_FLAG_CDC_TX_BUSY) {
        return;
    }
    
    // Configure the transmission descriptor directly to the banner text
    ps->cdc_tx_desc[0].buf = (char*)banner_text;
    ps->cdc_tx_desc[0].len = sizeof(banner_text) - 1;  // Exclude null terminator
    
    ps->flags |= PROG_FLAG_CDC_TX_BUSY;
    
    // Attempt to send the banner
    if (platform_usart_cdc_tx_async(ps->cdc_tx_desc, 1)) {
        ps->flags &= ~PROG_FLAG_BANNER_PENDING;
        ps->flags &= ~PROG_FLAG_CDC_TX_BUSY; // Clear busy flag on success
        ps->banner_displayed = true;
    } else {
        // Keep flags set to retry later, but clear CDC_TX_BUSY after 100ms to prevent deadlock
        platform_timespec_t current_time;
        platform_tick_count(&current_time);
        ps->last_display_timestamp = current_time.nr_sec * 1000000000 + current_time.nr_nsec;
    }
}

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
                                     char *raw_sentence_buf) {
    // Check if CDC TX is available
    if (platform_usart_cdc_tx_busy()) {
        return false;
    }
    
    if (ps->flags & PROG_FLAG_CDC_TX_BUSY) {
        // Check for potential stuck flag condition
        platform_timespec_t current_time;
        platform_tick_count(&current_time);
        uint64_t current_ns = current_time.nr_sec * 1000000000ULL + current_time.nr_nsec;
        uint64_t last_ns = ps->last_display_timestamp;
        
        // If flag has been set for more than 1 second, clear it
        if (current_ns - last_ns > 1000000000ULL) {
            ps->flags &= ~PROG_FLAG_CDC_TX_BUSY;
        } else {
            return false;
        }
    }
    
    // Format the GPS data with color
    int len = snprintf(ps->cdc_tx_buf, CDC_TX_BUF_SZ,
                     "%s[GPS] Time: %s%s | Lat: %s%s | Lon: %s%s\r\n",
                     ANSI_GREEN, ANSI_BOLD, time_str, 
                     ANSI_RESET, lat_str, 
                     ANSI_RESET, lon_str);
    
    // Check if formatting was successful
    if (len <= 0 || len >= CDC_TX_BUF_SZ) {
        return false;
    }
    
    // Configure the transmission descriptor
    ps->cdc_tx_desc[0].buf = ps->cdc_tx_buf;
    ps->cdc_tx_desc[0].len = len;
    
    ps->flags |= PROG_FLAG_CDC_TX_BUSY;
    
    // Update timestamp for deadlock detection
    platform_timespec_t current_time;
    platform_tick_count(&current_time);
    ps->last_display_timestamp = current_time.nr_sec * 1000000000ULL + current_time.nr_nsec;
    
    // Attempt to send the GPS data
    if (platform_usart_cdc_tx_async(ps->cdc_tx_desc, 1)) {
        ps->flags &= ~PROG_FLAG_CDC_TX_BUSY;
        ps->flags &= ~PROG_FLAG_GPGLL_DATA_PARSED;
        
        // Clear the source buffer if provided
        if (raw_sentence_buf) {
            raw_sentence_buf[0] = '\0';
        }
        
        return true;
    } else {
        // Keep TX busy flag set for retry
        return false;
    }
}

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
                                    uint16_t pm10) {
    // Check if CDC TX is available
    if (platform_usart_cdc_tx_busy()) {
        return false;
    }
    
    if (ps->flags & PROG_FLAG_CDC_TX_BUSY) {
        // Check for potential stuck flag condition
        platform_timespec_t current_time;
        platform_tick_count(&current_time);
        uint64_t current_ns = current_time.nr_sec * 1000000000ULL + current_time.nr_nsec;
        uint64_t last_ns = ps->last_display_timestamp;
        
        // If flag has been set for more than 1 second, clear it
        if (current_ns - last_ns > 1000000000ULL) {
            ps->flags &= ~PROG_FLAG_CDC_TX_BUSY;
        } else {
            return false;
        }
    }
    
    // Format the PM data with color (using ASCII for compatibility)
    int len = snprintf(ps->cdc_tx_buf, CDC_TX_BUF_SZ,
                     "%s[PM] PM1.0: %u ug/m3 | PM2.5: %u ug/m3 | PM10: %u ug/m3\r\n",
                     ANSI_CYAN, pm1_0, pm2_5, pm10);
    
    // Check if formatting was successful
    if (len <= 0 || len >= CDC_TX_BUF_SZ) {
        return false;
    }
    
    // Configure the transmission descriptor
    ps->cdc_tx_desc[0].buf = ps->cdc_tx_buf;
    ps->cdc_tx_desc[0].len = len;
    
    ps->flags |= PROG_FLAG_CDC_TX_BUSY;
    
    // Update timestamp for deadlock detection
    platform_timespec_t current_time;
    platform_tick_count(&current_time);
    ps->last_display_timestamp = current_time.nr_sec * 1000000000ULL + current_time.nr_nsec;
    
    // Attempt to send the PM data
    if (platform_usart_cdc_tx_async(ps->cdc_tx_desc, 1)) {
        ps->flags &= ~PROG_FLAG_CDC_TX_BUSY;
        ps->flags &= ~PROG_FLAG_PM_DATA_PARSED;
        return true;
    } else {
        // Keep TX busy flag set for retry
        return false;
    }
}

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
                                         uint16_t pm10) {
    // Check if CDC TX is available
    if (platform_usart_cdc_tx_busy()) {
        return false;
    }
    
    if (ps->flags & PROG_FLAG_CDC_TX_BUSY) {
        return false;
    }
    
    // Check if GPS data is available or empty
    bool has_gps_data = (time_str && time_str[0] != '\0' && 
                         lat_str && lat_str[0] != '\0' && 
                         lon_str && lon_str[0] != '\0');
    
    int len;
    
    // Format the combined data with simplified ANSI color formatting
    if (has_gps_data) {
        // GPS data available - display with simple formatting
        len = snprintf(ps->cdc_tx_buf, CDC_TX_BUF_SZ,
                     "%s[GPS] Time: %s%s | Lat: %s | Lon: %s  %s[PM] PM1.0: %u ug/m3 | PM2.5: %u ug/m3 | PM10: %u ug/m3%s\r\n",
                     ANSI_GREEN, ANSI_BOLD, time_str, 
                     lat_str, 
                     lon_str,
                     ANSI_CYAN, pm1_0, pm2_5, pm10, ANSI_RESET);
    } else {
        // GPS data not available - display waiting message
        len = snprintf(ps->cdc_tx_buf, CDC_TX_BUF_SZ,
                     "%s[GPS] Waiting for data...  %s[PM] PM1.0: %u ug/m3 | PM2.5: %u ug/m3 | PM10: %u ug/m3%s\r\n",
                     ANSI_GREEN, 
                     ANSI_CYAN, pm1_0, pm2_5, pm10, ANSI_RESET);
    }
    
    // Check if formatting was successful
    if (len <= 0 || len >= CDC_TX_BUF_SZ) {
        return false;
    }
    
    // Configure the transmission descriptor
    ps->cdc_tx_desc[0].buf = ps->cdc_tx_buf;
    ps->cdc_tx_desc[0].len = len;
    
    ps->flags |= PROG_FLAG_CDC_TX_BUSY;
    
    // Attempt to send the combined data
    if (platform_usart_cdc_tx_async(ps->cdc_tx_desc, 1)) {
        ps->flags &= ~PROG_FLAG_CDC_TX_BUSY;
        ps->flags &= ~PROG_FLAG_GPGLL_DATA_PARSED;
        ps->flags &= ~PROG_FLAG_PM_DATA_PARSED;
        ps->flags &= ~PROG_FLAG_COMBINED_DISPLAY_READY;
        
        return true;
    } else {
        // Keep TX busy flag set for retry
        return false;
    }
}

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
                                     size_t raw_data_len) {
    // Check if CDC TX is available
    if (platform_usart_cdc_tx_busy()) {
        return false;
    }
    
    if (ps->flags & PROG_FLAG_CDC_TX_BUSY) {
        return false;
    }
    
    // Apply different formatting for GPS vs PM data
    if (prefix && strncmp(prefix, "GPS", 3) == 0) {
        // GPS data often has special formatting needs - use a dedicated buffer format
        size_t formatted_len = 0;
        
        // Start with the header in green
        formatted_len = snprintf(ps->cdc_tx_buf, CDC_TX_BUF_SZ, "\033[32m[%s] \033[0m", prefix);
        
        // For NMEA sentences (which should end with \r\n), replace them with explicit newline
        // and ensure any control characters are displayed visibly
        for (size_t i = 0; i < raw_data_len && formatted_len < (CDC_TX_BUF_SZ - 10); i++) {
            char c = raw_data_str[i];
            
            // Special handling for control characters
            if (c == '\r') {
                // Skip \r for cleaner terminal display
                continue;
            } else if (c == '\n') {
                // Add a real newline for terminal display
                formatted_len += snprintf(ps->cdc_tx_buf + formatted_len, 
                                         CDC_TX_BUF_SZ - formatted_len, 
                                         "\r\n");
            } else {
                // Normal character
                ps->cdc_tx_buf[formatted_len++] = c;
            }
        }
        
        // Ensure null termination
        ps->cdc_tx_buf[formatted_len] = '\0';
        
        // Configure the transmission descriptor
        ps->cdc_tx_desc[0].buf = ps->cdc_tx_buf;
        ps->cdc_tx_desc[0].len = formatted_len;
    } else {
        // Standard handling for other raw data types
        // Calculate the total needed length with prefix
        size_t prefix_len = (prefix) ? strlen(prefix) + 2 : 0; // +2 for ": "
        size_t total_len = prefix_len + raw_data_len;
        
        // Ensure the data will fit in the buffer
        if (total_len >= CDC_TX_BUF_SZ) {
            // Truncate if necessary
            raw_data_len = CDC_TX_BUF_SZ - prefix_len - 1;
            total_len = prefix_len + raw_data_len;
        }
        
        // Copy data to the buffer, with prefix if provided
        if (prefix && prefix_len > 0) {
            snprintf(ps->cdc_tx_buf, CDC_TX_BUF_SZ, "\033[33m[%s] \033[0m", prefix);
            memcpy(ps->cdc_tx_buf + prefix_len, raw_data_str, raw_data_len);
        } else {
            memcpy(ps->cdc_tx_buf, raw_data_str, raw_data_len);
        }
        
        // Add a trailing newline if there isn't one already
        if (raw_data_len > 0 && ps->cdc_tx_buf[total_len-2] != '\r' && ps->cdc_tx_buf[total_len-1] != '\n') {
            ps->cdc_tx_buf[total_len++] = '\r';
            ps->cdc_tx_buf[total_len++] = '\n';
        }
        
        // Ensure null termination
        ps->cdc_tx_buf[total_len] = '\0';
        
        // Configure the transmission descriptor
        ps->cdc_tx_desc[0].buf = ps->cdc_tx_buf;
        ps->cdc_tx_desc[0].len = total_len;
    }
    
    ps->flags |= PROG_FLAG_CDC_TX_BUSY;
    
    // Attempt to send the raw data
    if (platform_usart_cdc_tx_async(ps->cdc_tx_desc, 1)) {
        ps->flags &= ~PROG_FLAG_CDC_TX_BUSY;
        return true;
    } else {
        // Keep TX busy flag set for retry
        return false;
    }
} 