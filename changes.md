# START

how: Added a new static function `debug_print_hex` to format and print byte arrays as hexadecimal strings. This function handles potential UART busy states and splits long hexdumps into multiple transmissions if necessary. Called this function from `process_accumulated_pm_data` immediately after the initial "Processing accumulated" debug message.
where:
  - File: `src/main.c`
  - New function: `debug_print_hex` (lines approx. 43-110, exact lines depend on final formatting after apply)
  - Call location: Within `process_accumulated_pm_data` function, after the `debug_printf` for "PM: Processing accumulated %u bytes". (around line 126)
why: To provide a raw hexadecimal view of the accumulated PM sensor data for debugging purposes, as requested by the user.
what: Implemented a hexdump output for the `pm_accumulate_buffer` content when `process_accumulated_pm_data` is called. The output is prefixed with "[DEBUG] PM: the hexdump (N bytes):" followed by the space-separated hex values and a newline.

---

how: Modified the `debug_print_hex` function to improve UART serialization and output accuracy.
where:
  - File: `src/main.c`
  - Function: `debug_print_hex` (lines approx. 43-105 of the modified file)
why: The previously added hexdump was not appearing in the output. This was likely due to the function returning prematurely because of UART busy checks that conflicted with preceding debug messages. This modification makes the hexdump function wait for the hardware UART to be free before attempting to print and ensures the output format includes the `[DEBUG]` prefix as intended.
what:
  - Removed the initial aggressive guard condition from `debug_print_hex`.
  - Ensured the function first waits for `platform_usart_cdc_tx_busy()` to be false before any print attempt.
  - Prepended `[DEBUG] ` to the hexdump\'s header string, e.g., `[DEBUG] PM: the hexdump (%u bytes):\\r\\n`.
  - Removed the specific `if (len == 0)` block that called `debug_printf`, as the main logic now correctly handles the `len == 0` case by printing the modified header.
  - Refined buffer full check and line ending logic for clarity and robustness.

---

how: Modified the GPS data processing logic to print raw GPS data in ASCII format, prefixed with a specific label, and ensured robust UART handling.
where:
  - File: `src/main.c`
  - Function: `prog_loop_one`
  - Specific block: Inside `if (app_state.gps_rx_desc.compl_type == PLATFORM_USART_RX_COMPL_DATA)`, the conditional block `if (DEBUG_MODE_RAW_GPS || PM_FORCE_RAW_GPS)` was updated (around lines 360-400 of the modified file).
why: To meet the user requirement of displaying raw GPS NMEA sentences in ASCII format directly in the debug output, prefixed by `[DEBUG] GPS Raw Data:`, for easier monitoring and debugging of GPS communication.
what:
  - Replaced the previous raw GPS data printing mechanism (`ui_handle_raw_data_transmission` with "GPS RAW BUFFER" label).
  - Added a call to `debug_printf(&app_state, \"GPS Raw Data:\");` to print the desired label.
  - Implemented a new loop to transmit the `app_state.gps_assembly_buf` (raw GPS data) in chunks.
  - Each chunk is copied to `app_state.cdc_tx_buf` and sent via `platform_usart_cdc_tx_async`.
  - Robust UART busy checks (`platform_usart_cdc_tx_busy()` with timeouts and `platform_do_loop_one()`) are implemented before printing the label and before sending each chunk of raw GPS data to prevent data loss or corruption due to UART contention.
  - The chunk size for GPS data is dynamically determined based on `CDC_TX_BUF_SZ` to maximize throughput while ensuring safe buffer handling.

---

how: Implemented a dedicated function `debug_print_gps_raw_data` for robustly printing a label and then chunked raw GPS data, managing UART busy states carefully throughout the process.
where:
  - File: `src/main.c`
  - New function: `debug_print_gps_raw_data` (defined before `debug_print_hex`).
  - Call location: In `prog_loop_one`, within the GPS data handling section (`if (DEBUG_MODE_RAW_GPS || PM_FORCE_RAW_GPS)`), replacing the previous simpler GPS raw print logic.
why: The previous attempts to print raw GPS data with its label were unreliable due to UART contention and the early-exit behavior of `debug_printf`. A dedicated function with UART management similar to the working `debug_print_hex` was needed.
what:
  - Created `debug_print_gps_raw_data(prog_state_t *ps, const char *raw_data_buffer, uint16_t raw_data_len)`.
  - This function first waits for the UART hardware to be free.
  - It then formats and transmits the label `[DEBUG] GPS Raw Data:\\r\\n` using `ps->cdc_tx_buf` and `platform_usart_cdc_tx_async`.
  - It waits for the label\'s hardware transmission to complete.
  - It then transmits the `raw_data_buffer` (ASCII GPS data) in chunks, using `ps->cdc_tx_buf`.
  - Each step (label transmission, each data chunk transmission) includes waiting for UART hardware to be free before attempting to send and waiting for hardware transmission to complete afterwards.
  - The `PROG_FLAG_CDC_TX_BUSY` software flag is managed (set before `platform_usart_cdc_tx_async`, cleared if `platform_usart_cdc_tx_async` starts successfully or fails to start) for each transmission part, consistent with `debug_print_hex`.
  - The previous complex chunking logic and `debug_printf` call for GPS raw data in `prog_loop_one` were replaced by a single call to this new robust function.

---

how: Made `debug_printf` effectively synchronous by ensuring it waits for true UART idle on entry, and after starting an async send, it waits for that specific hardware transmission to complete before clearing its own software busy flag and returning.
where:
  - File: `src/main.c`
  - Function: `debug_printf`
why: Persistent issues with multi-part debug messages (especially raw GPS data) not appearing reliably, indicating that previous UART serialization attempts were insufficient. This change aims to make each `debug_printf` call a fully blocking operation from the caller\'s perspective, guaranteeing that one print completes before the next can start, thus preventing UART contention and buffer overwrites.
what:
  - Modified `debug_printf` to include an entry wait loop: `while(((ps->flags & PROG_FLAG_CDC_TX_BUSY) || platform_usart_cdc_tx_busy()) && entry_wait_timeout-- > 0) { platform_do_loop_one(); }`. If the timeout is reached and UART is still busy, the function returns early.
  - After formatting the message and before calling `platform_usart_cdc_tx_async`, `ps->flags |= PROG_FLAG_CDC_TX_BUSY;` is set.
  - If `platform_usart_cdc_tx_async` returns `true` (successfully started):
    - A new hardware wait loop `while(platform_usart_cdc_tx_busy() && hardware_wait_timeout-- > 0) { platform_do_loop_one(); }` is entered.
    - After this loop (or if it times out), `ps->flags &= ~PROG_FLAG_CDC_TX_BUSY;` is executed, effectively making this `debug_printf` call manage its own software busy flag for its duration.
  - If `platform_usart_cdc_tx_async` returns `false` (failed to start), `ps->flags &= ~PROG_FLAG_CDC_TX_BUSY;` is executed immediately.
  - The helper functions `debug_print_hex` and `debug_print_gps_raw_data` (which were already simplified to call `debug_printf` per line/segment) remain unchanged, as they will now benefit from the synchronous behavior of `debug_printf`.

---

how: Added diagnostic prints to the GPS data handling logic and ensured the `gps_rx_desc.compl_type` flag is reset after processing.
where:
  - File: `src/main.c`
  - Function: `prog_loop_one` (within the `if (app_state.gps_rx_desc.compl_type == PLATFORM_USART_RX_COMPL_DATA)` block)
why: To diagnose why raw GPS data was not being printed, by tracing the data flow and ensuring the receive completion flag is properly managed to detect new incoming data.
what:
  - Added several `debug_printf` calls to log:
    - Detection of `PLATFORM_USART_RX_COMPL_DATA` for GPS, including hardware-reported data length and current assembly buffer length.
    - State of `app_state.gps_assembly_len` before and after appending new data.
    - A specific message if `app_state.gps_assembly_len` is zero when `debug_print_gps_raw_data` is about to be called.
    - Details of GPS assembly buffer overflow events, including clearing and resetting the buffer.
    - Information about NMEA parsing consumption.
  - Added `app_state.gps_rx_desc.compl_type = PLATFORM_USART_RX_COMPL_NONE;` at the end of the GPS data processing block. This is critical for allowing the system to correctly identify subsequent incoming GPS data packets.

---

how: Added a diagnostic print statement to continuously monitor the state of the GPS receiver's completion type.
where:
  - File: `src/main.c`
  - Function: `prog_loop_one`
  - Specific location: Immediately before the `if (app_state.gps_rx_desc.compl_type == PLATFORM_USART_RX_COMPL_DATA)` check within the GPS Data Handling section.
why: To understand why the GPS raw data print is not appearing. This diagnostic will show the state of `app_state.gps_rx_desc.compl_type` and associated lengths in every iteration of `prog_loop_one`, helping to determine if data reception events are being signaled correctly by the platform layer or if the RX mechanism is stuck.
what: Inserted a `debug_printf` call: `debug_printf(&app_state, "GPS Loop Check: compl_type=%d, hw_len=%u, assembly_len=%u", app_state.gps_rx_desc.compl_type, app_state.gps_rx_desc.compl_info.data_len, app_state.gps_assembly_len);`. This will provide continuous insight into the GPS RX descriptor's status.

---

how: Ensured USART RX descriptors are fully reset before re-arming receive operations. This involves clearing the `compl_info` structure and re-initializing `buf` and `max_len` fields of the `platform_usart_rx_desc_t` for both GPS and PM peripherals.
where:
  - File: `src/main.c`
  - Function: `prog_loop_one`
    - Inside the GPS data handling block (`if (app_state.gps_rx_desc.compl_type == PLATFORM_USART_RX_COMPL_DATA)`), before calling `gps_platform_usart_cdc_rx_async`.
    - Inside the PM data handling block (`if (app_state.pm_rx_desc.compl_type == PLATFORM_USART_RX_COMPL_DATA)`), before calling `pm_platform_usart_cdc_rx_async`.
    - Within the watchdog timer logic, before re-arming GPS RX (`if (gps_platform_usart_cdc_rx_busy())`).
    - Within the watchdog timer logic, before re-arming PM RX (`if (pm_platform_usart_cdc_rx_busy())`).
why: To address persistent issues where GPS data reception was not being signaled (`compl_type` remained 0) and PM data reception was incomplete. This change aims to prevent stale state in the RX descriptors from interfering with new reception attempts, potentially resolving hangs or malfunctions in the platform's USART RX mechanism.
what: Added `memset((void*)&app_state.X_rx_desc.compl_info, 0, sizeof(app_state.X_rx_desc.compl_info));` and ensured `app_state.X_rx_desc.buf` and `app_state.X_rx_desc.max_len` are correctly set immediately before each call to `gps_platform_usart_cdc_rx_async` and `pm_platform_usart_cdc_rx_async` in the identified locations. Moved the `app_state.gps_rx_desc.compl_type = PLATFORM_USART_RX_COMPL_NONE;` line to be before the re-arming logic in the GPS data handler for consistency.

---

how: Reduced the frequency of a verbose diagnostic print statement to lessen CPU load from debug messaging.
where:
  - File: `src/main.c`
  - Function: `prog_loop_one`
  - Specific location: The `debug_printf` call for "GPS Loop Check" (around line 405-407).
why: The constant printing of "GPS Loop Check" in every loop iteration by the synchronous `debug_printf` was suspected to consume excessive processing time, potentially starving other critical tasks like actual data reception and processing. Throttling this message aims to alleviate this pressure.
what: Introduced a static counter `gps_loop_check_counter`. The "GPS Loop Check" message is now printed only when this counter reaches 1000, after which the counter is reset. The message text was also updated to "GPS Loop Check (throttled):" to reflect this change.

---

how: Significantly reduced debug message volume by making most low-level/verbose debug prints conditional using a preprocessor macro.
where:
  - File: `src/main.c`
  - Macro definition: Added `#define DEBUG_LEVEL_VERBOSE 0` near the top.
  - Conditional blocks: Wrapped numerous `debug_printf` calls with `#if DEBUG_LEVEL_VERBOSE ... #endif` in:
    - `process_accumulated_pm_data` (for hexdump, header validation, raw hex preparation details).
    - `prog_loop_one` within the GPS data handling section (for assembly details, buffer overflow, NMEA parsing consumption, call to `debug_print_gps_raw_data` trigger).
    - `prog_loop_one` within GPGLL parsing success message.
why: To alleviate excessive CPU load caused by frequent synchronous `debug_printf` calls, which was suspected of hindering timely processing of USART interrupts and data, leading to slow operation and failure to detect GPS data.
what: Introduced `DEBUG_LEVEL_VERBOSE` macro, initialized to `0`. Most detailed, high-frequency debug messages are now only compiled if this macro is set to `1`. Key higher-level messages (e.g., "GPS: RX_COMPL_DATA event", "PM: Parsed OK") remain active.

---

how: Drastically simplified `prog_loop_one` to isolate GPS reception by temporarily disabling PM sensor processing and complex GPS data handling (assembly, parsing). If a GPS RX completion event occurs, the raw hardware buffer is now printed directly.
where:
  - File: `src/main.c`
  - Function: `prog_loop_one`
    - Commented out the PM accumulation timeout block.
    - Commented out the entire PM sensor data handling block (`if (app_state.pm_rx_desc.compl_type == PLATFORM_USART_RX_COMPL_DATA)`).
    - Modified the GPS data handling block (`if (app_state.gps_rx_desc.compl_type == PLATFORM_USART_RX_COMPL_DATA)`):
        - Retained initial "GPS: RX_COMPL_DATA event..." debug print (modified to only show `hw_len`).
        - Added new debug print: "GPS RAW DATA DETECTED - PRINTING DIRECT BUFFER CONTENTS".
        - Added call to `debug_print_gps_raw_data(&app_state, (const char*)app_state.gps_rx_buf, app_state.gps_rx_desc.compl_info.data_len);`.
        - Commented out all subsequent GPS assembly and NMEA parsing logic within this block.
why: To minimize system load and isolate basic GPS data reception. This tests if the fundamental `PLATFORM_USART_RX_COMPL_DATA` event for GPS is being triggered and if data can be printed directly from the hardware buffer, bypassing potential issues in more complex data handling paths or resource starvation due to other processing/debugging activities.
what: Focused `prog_loop_one` almost exclusively on detecting and printing raw GPS hardware buffer content upon RX completion. Most other processing, especially PM sensor handling and detailed GPS parsing, is temporarily disabled.

---

how: Added missing pin multiplexing configuration for SERCOM1 (GPS USART) pins PB16 (TX) and PB17 (RX) within the `gps_platform_usart_init` function.
where:
  - File: `src/drivers/gps_usart.c`
  - Function: `gps_platform_usart_init`
  - Specific location: Before the final step of enabling the SERCOM1 peripheral (before `UART_REGS->SERCOM_CTRLA |= (0x1 << 1);`).
why: The `PB_init()` function in `platform/gpio.c` was found to only configure pins for the push button (PA23) and did not handle the multiplexing for SERCOM1 pins. Without correct pin multiplexing, SERCOM1 cannot communicate with the external GPS module, leading to no data reception (`compl_type` remaining 0). This change restores the pin configuration logic present in the original standalone GPS example.
what: Inserted PORT register configurations to:
  - Set PB17 (RXD) direction to input and enable its input buffer and peripheral multiplexer.
  - Set PB16 (TXD) direction to output and enable its peripheral multiplexer.
  - Set `PORT_SEC_REGS->GROUP[1].PORT_PMUX[8]` to connect PB16 (PMUXE) and PB17 (PMUXO) to peripheral function D (0x3), which corresponds to SERCOM1 for these pins.

---

how: Corrected the relative include path for `platform.h` in `src/drivers/gps_usart.c`.
where:
  - File: `src/drivers/gps_usart.c`
  - Line: Around 39 (the `#include` directive for `platform.h`).
why: The previous path (`"platform.h"`) was incorrect for the file's location within `src/drivers/`, potentially leading to compilation errors or misconfigurations if the wrong `platform.h` (or none at all) was found. The `platform.h` file resides in the project's `inc` directory.
what: Changed `#include "platform.h"` to `#include "../../inc/platform.h"`.

---

how: Increased the USART RX idle timeout for the GPS module (SERCOM1) to 50 milliseconds.
where:
  - File: `src/drivers/gps_usart.c`
  - Function: `gps_platform_usart_init`
  - Specific line: The assignment to `gps_ctx_uart.cfg.ts_idle_timeout.nr_nsec`.
why: The system was detecting GPS RX completion after each individual byte. This suggests the previous idle timeout (approx. 781 µs) was too short, causing the reception to finalize prematurely before a complete NMEA sentence could be accumulated. A longer timeout provides more opportunity to gather a meaningful string of bytes.
what: Changed `gps_ctx_uart.cfg.ts_idle_timeout.nr_nsec` from `781250` (0.78 ms) to `50000000` (50 ms).

---

how: Increased the size of the GPS hardware receive buffer.
where:
  - File: `inc/main.h`
  - Macro: `GPS_RX_BUF_SZ`
why: The previously configured buffer size (128 bytes) was insufficient to hold complete NMEA sentences, leading to truncated raw GPS data output. Increasing the buffer size allows for the reception of longer data strings from the GPS module before the buffer full condition is met in the USART driver.
what: Changed the `GPS_RX_BUF_SZ` macro definition from `128` to `512`.

---

how: Restored PM and GPS data processing logic and refined debug output.
where:
  - File: `src/main.c`
  - Functions: `prog_loop_one`, `process_accumulated_pm_data`, `ui_display_combined_data`
why: To revert to full application functionality after a period of focused GPS RX debugging, and to meet the user's desired debug output format, which includes PM hexdump, raw GPS data, and final parsed values, while removing intermediate diagnostic messages.
what:
  - Uncommented and restored the PM sensor data accumulation timeout logic and the main PM data handling block in `prog_loop_one`.
  - Uncommented and restored the GPS data assembly and NMEA parsing logic within the GPS data handling block in `prog_loop_one`.
  - Ensured `DEBUG_MODE_RAW_PM` controls the PM hexdump via `debug_print_hex` in `process_accumulated_pm_data`.
  - Ensured `DEBUG_MODE_RAW_GPS` controls the raw GPS data print via `debug_print_gps_raw_data` in `prog_loop_one`.
  - Removed diagnostic `debug_printf` calls: "GPS: RX_COMPL_DATA event. Hardware len: ..." and "GPS RAW DATA DETECTED - PRINTING DIRECT BUFFER CONTENTS" from `prog_loop_one`.
  - Verified that `ui_display_combined_data` is active for printing the final parsed data.
  - Ensured `DEBUG_LEVEL_VERBOSE` remains available to control finer-grained diagnostic messages if needed in the future, but the primary requested outputs are now independent of it (controlled by `DEBUG_MODE_RAW_PM` and `DEBUG_MODE_RAW_GPS`).

---

how: Corrected various linter errors that arose after restoring PM/GPS processing logic.
where:
  - File: `src/main.c`
  - Various functions including `process_accumulated_pm_data`, `prog_loop_one`, and `ui_display_combined_data`.
why: To resolve build-breaking errors and warnings reported by the linter, ensuring the code is compilable and behaves as intended.
what:
  - Changed `ps->pm_accumulate_len` to `pm_accumulate_len` and `ps->pm_accumulate_buffer` to `pm_accumulate_buffer` as these are static global variables.
  - Corrected the arguments for the `debug_print_hex` function call in `process_accumulated_pm_data`.
  - Replaced uses of the undefined `PLATFORM_USART_RX_COMPL_IDLE_TIMEOUT` with `PLATFORM_USART_RX_COMPL_DATA` in PM data handling.
  - Removed the duplicate definition of the static variable `gps_loop_check_counter`.
  - Added forward declarations for `debug_printf` and `ui_display_combined_data` to resolve implicit declaration warnings.
  - Added a definition for `UART_WAIT_TIMEOUT_COUNT`.
  - Corrected the member access in `ui_display_combined_data` from `ps->latest_pms_data.pm10_0_std` to `ps->latest_pms_data.pm10_std`.
  - Removed the duplicate definition of `PM_FORCE_RAW_GPS`.
  - Ensured PM accumulation buffer size checks use `sizeof(pm_accumulate_buffer)` instead of `PM_RX_BUF_SZ` for correctness.

---

how: Corrected the NMEA GPGLL parsing function call in `src/main.c` to use the existing `nmea_parse_gpgll_and_format` function and updated data handling accordingly.
where:
  - File: `inc/main.h` (updated `prog_state_t`)
  - File: `src/main.c` (updated GPGLL parsing logic in `prog_loop_one` and `ui_display_combined_data`)
  - Reverted previous incorrect changes to `inc/parsers/nmea_parser.h` and `src/parsers/nmea_parse.c`.
why: To resolve the "undefined reference to `nmea_parse_gpggl`" linker error by using the correctly named and implemented NMEA parsing function (`nmea_parse_gpgll_and_format`).
what:
  - Removed the erroneously added prototype for `nmea_parse_gpggl_fields` from `inc/parsers/nmea_parser.h`.
  - Removed the erroneously added implementation of `nmea_parse_gpggl_fields` from `src/parsers/nmea_parse.c`.
  - Added a new field `formatted_gpggl_string` to `prog_state_t` in `inc/main.h` to store the output of `nmea_parse_gpgll_and_format`.
  - Modified the GPGLL sentence processing in `prog_loop_one` in `src/main.c` to call `nmea_parse_gpgll_and_format` and store its result into `app_state.formatted_gpggl_string`.
  - Updated `ui_display_combined_data` in `src/main.c` to use `app_state.formatted_gpggl_string` for displaying GPS data, instead of separate time, lat, lon fields.

---

how: Added an include directive for `parsers/nmea_parser.h` in `inc/main.h`.
where:
  - File: `inc/main.h`
  - Line: Added after the include for `parsers/pms_parser.h`.
why: To resolve "undeclared here (not in a function)" errors for `NMEA_PARSER_MAX_COORD_STR_LEN` and `NMEA_PARSER_MAX_TIME_STR_LEN`. These macros are defined in `nmea_parser.h` and were used in `main.h` without the necessary include.
what: Added `#include "parsers/nmea_parser.h"` to `inc/main.h`.

---

how: Refined PM data accumulation logic and corrected debug message formatting.
where:
  - File: `src/main.c`
  - Function: `prog_loop_one` (PM data accumulation and timeout logic)
  - Function: `debug_print_gps_raw_data` (corrected label printing)
  - Function: `ui_display_combined_data` (adjusted format string for combined output and switched to PM ATM values)
why: To fix issues where PM hexdump showed only single bytes (incomplete accumulation) and to align debug outputs more closely with user's desired format, including removing double `[DEBUG]` prefixes and adjusting combined data presentation.
what:
  - Restructured PM data handling in `prog_loop_one`: data is accumulated in `pm_accumulate_buffer` upon `PLATFORM_USART_RX_COMPL_DATA`. `pm_last_receive_time` is updated. `process_accumulated_pm_data` is now called if `pm_accumulate_len` reaches `PM_ACCUMULATE_THRESHOLD` OR if `PM_ACCUMULATE_TIMEOUT_MS` elapses with `pm_accumulate_len > 0`. The buffer is cleared after processing.
  - Corrected `debug_print_gps_raw_data`: changed `debug_printf(ps, "[DEBUG] GPS Raw Data:\r\n");` to `debug_printf(ps, "GPS Raw Data:");` as `debug_printf` already adds `[DEBUG]` and CRLF.
  - Modified `ui_display_combined_data`: changed the `debug_printf` format string to `"GPS: %s | PM: PM1.0: %u, PM2.5: %u, PM10: %u"` to remove the redundant `[GPS]` and `[PM]` prefixes from the *content* (as `debug_printf` adds `[DEBUG]`). Switched from `latest_pms_data.pmX_Y_std` to `latest_pms_data.pmX_Y_atm` for displaying PM values.

---
how: Increased the PM sensor USART (SERCOM0) reception idle timeout and corrected include path.
where:
  - File: `src/drivers/pm_usart.c`
  - Function: `pm_platform_usart_init`
  - Line: `pm_ctx_uart.cfg.ts_idle_timeout.nr_nsec` assignment and the `#include "platform.h"` line.
why: The PM sensor data was being received in small, incomplete chunks (often just 1 byte, 0x42), causing the hexdump to be partial. This was due to a very short USART RX idle timeout (0.78125 ms) which is less than the transmission time of a single byte at 9600 baud. Increasing this timeout allows the driver to accumulate more bytes, ideally a full packet, before signaling reception completion. Also, the include path for `platform.h` was incorrect for its location.
what:
  - Changed `pm_ctx_uart.cfg.ts_idle_timeout.nr_nsec` from `781250` to `50000000` (50 milliseconds).
  - Updated the comment for this line to reflect the new value and rationale.
  - Corrected `#include "platform.h"` to `#include "../../inc/platform.h"`.
  - Ensured PM sensor uses 1 stop bit (CTRLB.SBMODE=0) as per typical PMS5003 configuration.
  - Verified PA05 (RX) pin multiplexing for SERCOM0_ALT.

---
how: Modified the condition for displaying combined GPS/PM data to trigger primarily on parsed PM data availability.
where:
  - File: `src/main.c`
  - Function: `prog_loop_one`
  - Specific location: The "Combined Data Display Logic" block (around line 517).
why: To make the combined data line (e.g., `[DEBUG] GPS: ... | PM: ...`) print more consistently, especially when PM data is available but GPS data might still be "Waiting for data...". The previous condition required both GPS and PM data to be parsed, leading to infrequent updates if GPS had no fix.
what:
  - Changed the primary condition for calling `ui_display_combined_data` from `(app_state.flags & PROG_FLAG_GPGLL_DATA_PARSED) && (app_state.flags & PROG_FLAG_PM_DATA_PARSED)` to `(app_state.flags & PROG_FLAG_PM_DATA_PARSED)`.
  - Adjusted the flag clearing logic within this block: `PROG_FLAG_PM_DATA_PARSED` is always cleared after display. `PROG_FLAG_GPGLL_DATA_PARSED` is also cleared if it was set (meaning its data, valid or "N/A", was incorporated into the display by `ui_display_combined_data`). This ensures the display updates whenever new PM data is parsed and the display interval has elapsed, showing the latest PM values alongside the current GPS status.

---
how: Added ANSI color escape codes to the combined GPS/PM data display string.
where:
  - File: `src/main.c` (function `ui_display_combined_data`)
  - File: `inc/terminal_ui.h` (added extern declarations for new ANSI color constants)
  - File: `src/terminal_ui.c` (added definitions for new ANSI color constants)
why: To visually distinguish GPS data (violet) from PM data (gold/yellow) in the terminal output, as requested, and to centralize ANSI code definitions according to existing conventions.
what:
  - Added `extern const char ANSI_MAGENTA[];` and `extern const char ANSI_YELLOW[];` to `inc/terminal_ui.h`. Made `ANSI_RESET` extern as well.
  - Defined `const char ANSI_MAGENTA[] = "\033[35m";`, `const char ANSI_YELLOW[] = "\033[33m";` in `src/terminal_ui.c` and made `ANSI_RESET` non-static.
  - Removed local `#define`s for these colors from `src/main.c`.
  - Modified the `snprintf` calls within `ui_display_combined_data` in `src/main.c` to use `ANSI_MAGENTA`, `ANSI_YELLOW`, and `ANSI_RESET` (now accessed via `terminal_ui.h`) to color the GPS and PM sections of the output string.

---
how: Replaced the body of the `ui_display_combined_data` function with the version developed in the immediately preceding step, which incorporates ANSI colorization for GPS (violet) and PM (gold/yellow) data, using constants defined in and made extern from `terminal_ui.c` and `terminal_ui.h`.
where:
  - File: `src/main.c`
  - Function: `ui_display_combined_data` (entire body replaced).
why: To implement the requested colorization of the combined GPS/PM output line, ensuring consistency with how other ANSI escape codes are managed in the project.
what: The `ui_display_combined_data` function in `src/main.c` now uses `ANSI_MAGENTA`, `ANSI_YELLOW`, and `ANSI_RESET` (sourced from `terminal_ui.h`) to format the output string with colors for GPS and PM data segments before passing it to `debug_printf`. The flag clearing logic remains handled by the caller (`prog_loop_one`).

---
how: Standardized the display of GPS data in the combined output line to always show the "Time | Lat | Long" format, using placeholders when actual data is unavailable. This involved initializing the GPS data string and simplifying the display logic.
where:
  - File: `src/main.c`
  - Function: `prog_setup()` (added initialization for `app_state.formatted_gpggl_string`)
  - Function: `ui_display_combined_data()` (simplified to always use `app_state.formatted_gpggl_string`)
why: To ensure the combined GPS/PM display line consistently shows the "GPS: Time | Lat | Long | PM: ..." structure, using "Waiting for data..." placeholders for GPS fields when a fix is not yet available, instead of sometimes showing "GPS: Data N/A".
what:
  - In `prog_setup()`, `app_state.formatted_gpggl_string` is now initialized with "--:--:-- | Lat: Waiting for data..., - | Long: Waiting for data..., -".
  - In `ui_display_combined_data()`, the `if/else` logic that chose between `ps->formatted_gpggl_string` and "GPS: Data N/A" was removed. The function now always uses `ps->formatted_gpggl_string` for the GPS part of the output. The `nmea_parse_gpgll_and_format` function is responsible for populating this string correctly with either actual data or the "Waiting for data..." placeholders.

---
how: Created a new `direct_printf` function for printing messages without the `[DEBUG]` prefix and updated `ui_display_combined_data` to use it. The `direct_printf` was also refined to use a static buffer for `vsnprintf`, mirroring `debug_printf`.
where:
  - File: `src/main.c`
  - New function: `direct_printf` (definition and forward declaration).
  - Modified function: `direct_printf` (changed `local_format_buf` to `static direct_local_format_buf`, refined `final_len` and `final_msg_buf` handling).
  - Modified function: `ui_display_combined_data` (changed call from `debug_printf` to `direct_printf`).
why: To allow the main status line (combined GPS/PM data) to be printed without the `[DEBUG]` prefix, making it a clear status update. Using a static buffer in `direct_printf` for formatting aligns its memory usage pattern with `debug_printf` and avoids repeated large stack allocations.
what:
  - Implemented `direct_printf(prog_state_t *ps, const char *fmt, ...)` which formats a string (using a static internal buffer `static char direct_local_format_buf[CDC_TX_BUF_SZ];`) and sends it via CDC USART, ensuring `\r\n` line endings, but without adding `[DEBUG]`. It includes UART busy management similar to `debug_printf`.
  - Corrected `final_msg_buf` size and `final_len` calculation within `direct_printf` to robustly handle string construction and `\r\n` appending.
  - Changed the final print call in `ui_display_combined_data` from `debug_printf(ps, "%s", temp_format_buf);` to `direct_printf(ps, "%s", temp_format_buf);`.
