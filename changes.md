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
  - Prepended `[DEBUG] ` to the hexdump's header string, e.g., `[DEBUG] PM: the hexdump (%u bytes):\r\n`.
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
  - Added a call to `debug_printf(&app_state, "GPS Raw Data:");` to print the desired label.
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
  - It then formats and transmits the label `[DEBUG] GPS Raw Data:\r\n` using `ps->cdc_tx_buf` and `platform_usart_cdc_tx_async`.
  - It waits for the label's hardware transmission to complete.
  - It then transmits the `raw_data_buffer` (ASCII GPS data) in chunks, using `ps->cdc_tx_buf`.
  - Each step (label transmission, each data chunk transmission) includes waiting for UART hardware to be free before attempting to send and waiting for hardware transmission to complete afterwards.
  - The `PROG_FLAG_CDC_TX_BUSY` software flag is managed (set before `platform_usart_cdc_tx_async`, cleared if `platform_usart_cdc_tx_async` starts successfully or fails to start) for each transmission part, consistent with `debug_print_hex`.
  - The previous complex chunking logic and `debug_printf` call for GPS raw data in `prog_loop_one` were replaced by a single call to this new robust function.

---

how: Made `debug_printf` effectively synchronous by ensuring it waits for true UART idle on entry, and after starting an async send, it waits for that specific hardware transmission to complete before clearing its own software busy flag and returning.
where:
  - File: `src/main.c`
  - Function: `debug_printf`
why: Persistent issues with multi-part debug messages (especially raw GPS data) not appearing reliably, indicating that previous UART serialization attempts were insufficient. This change aims to make each `debug_printf` call a fully blocking operation from the caller's perspective, guaranteeing that one print completes before the next can start, thus preventing UART contention and buffer overwrites.
what:
  - Modified `debug_printf` to include an entry wait loop: `while(((ps->flags & PROG_FLAG_CDC_TX_BUSY) || platform_usart_cdc_tx_busy()) && entry_wait_timeout-- > 0) { platform_do_loop_one(); }`. If the timeout is reached and UART is still busy, the function returns early.
  - After formatting the message and before calling `platform_usart_cdc_tx_async`, `ps->flags |= PROG_FLAG_CDC_TX_BUSY;` is set.
  - If `platform_usart_cdc_tx_async` returns `true` (successfully started):
    - A new hardware wait loop `while(platform_usart_cdc_tx_busy() && hardware_wait_timeout-- > 0) { platform_do_loop_one(); }` is entered.
    - After this loop (or if it times out), `ps->flags &= ~PROG_FLAG_CDC_TX_BUSY;` is executed, effectively making this `debug_printf` call manage its own software busy flag for its duration.
  - If `platform_usart_cdc_tx_async` returns `false` (failed to start), `ps->flags &= ~PROG_FLAG_CDC_TX_BUSY;` is executed immediately.
  - The helper functions `debug_print_hex` and `debug_print_gps_raw_data` (which were already simplified to call `debug_printf` per line/segment) remain unchanged, as they will now benefit from the synchronous behavior of `debug_printf`.