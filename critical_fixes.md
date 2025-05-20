# Critical Fix Log

## Entry: 2025-05-20 20:02:40 UTC+8

### Problem
The system was not outputting the expected hexdump of the PM sensor data after the debug message "[DEBUG] PM: Processing accumulated ... bytes". Only the summary and parsed PM values were being displayed, with no raw hex output visible in the terminal.

### Thought Process & Investigation
1. **Initial Review:**
   - The code was already calling a `debug_print_hex` function after the accumulation debug message.
   - The function attempted to print the hexdump using the same debug/CDC mechanism as other debug messages.
   - However, the output was missing, indicating a possible race or buffer contention.

2. **Analysis of USART/CDC Protocol:**
   - The system uses buffered USART output with busy flags (`platform_usart_cdc_tx_busy()` and `PROG_FLAG_CDC_TX_BUSY`).
   - If a debug message is sent while the hardware or software buffer is busy, the message is dropped or skipped.
   - The original `debug_print_hex` function checked for busy state at the start and would return immediately if busy, causing the hexdump to be skipped if a previous debug message was still transmitting.

3. **Root Cause:**
   - The hexdump was not being printed because the function returned prematurely due to the busy state, especially after the preceding debug message ("Processing accumulated ... bytes").
   - The debug output system was not serializing multi-part debug output correctly under high-frequency or back-to-back debug prints.

### Solution & Fix Implementation
- **Function Redesign:**
  - The `debug_print_hex` function was rewritten to *wait* for the hardware UART to become free before attempting to print any part of the hexdump.
  - The initial guard (`if (platform_usart_cdc_tx_busy() || (ps->flags & PROG_FLAG_CDC_TX_BUSY)) return;`) was removed.
  - The function now uses a timeout-based wait loop to ensure the UART is ready before each transmission chunk.
  - The output format was also updated to prepend `[DEBUG]` to the hexdump header for consistency.
  - Buffer management was improved to ensure no trailing spaces and proper CRLF line endings.

- **Testing & Validation:**
  - After the fix, the output now correctly shows:
    ```
    [DEBUG] PM: Processing accumulated 32 bytes
    [DEBUG] PM: the hexdump (32 bytes):
    42 4D ... (hex values)
    [GPS] Waiting for data...  [PM] PM1.0: ...
    ```
  - The hexdump reliably appears after each accumulation event, confirming the fix.

### Lessons Learned
- When using buffered USART/CDC debug output, always serialize multi-part debug output and avoid dropping messages due to busy flags.
- For critical debug output, implement a wait-and-retry mechanism rather than skipping output on busy state.
- Always test debug output under real timing and load conditions to catch race conditions and buffer contention issues.

--- 

## Entry: 2025-05-20 20:25:20 UTC+8

### Problem
Previously, multi-part debug messages (like PM hexdumps and raw GPS data lines) were not printing reliably. Attempts to fix this by creating specialized printing functions with internal busy-waiting (`debug_print_hex`, `debug_print_gps_raw_data`) were only partially successful, with the PM hexdump eventually working but raw GPS data still failing to print.

### Thought Process & Investigation
1.  **Initial Success and Persistent Failure:** The PM hexdump was fixed by making its dedicated print function (`debug_print_hex`) more robust in handling UART busy states. However, applying a similar strategy to `debug_print_gps_raw_data` did not yield results for GPS data, suggesting a deeper issue with how multiple `debug_printf` calls (or calls to wrappers that internally call `debug_printf`) interact.
2.  **Revisiting UART Primitives (Simulated Datasheet Review):** The core problem seemed to be the non-atomic nature of `debug_printf` in relation to the `PROG_FLAG_CDC_TX_BUSY` software flag and the `platform_usart_cdc_tx_busy()` hardware status. If `platform_usart_cdc_tx_busy()` only indicates hardware FIFO/register readiness (e.g., Data Register Empty - DRE) but an asynchronous operation (like DMA) is still using the main transmit buffer (`ps->cdc_tx_buf`), the `prog_loop_one` logic could clear `PROG_FLAG_CDC_TX_BUSY` prematurely. A subsequent call to `debug_printf` would then see the UART as "idle" and overwrite `ps->cdc_tx_buf` while it was still being used by the previous, unfinished hardware operation.
3.  **Hypothesis:** The most reliable way to serialize output is to make `debug_printf` itself an effectively synchronous (blocking) operation. It must wait for true UART idle (both software flag and hardware state) before starting, and then, crucially, it must wait for its *own* specific hardware transmission to complete fully before it releases control and allows any other print operation to begin.

### Solution & Fix Implementation
-   **`debug_printf` Refactoring for Synchronous Behavior:**
    1.  **Entry Wait:** On entry, `debug_printf` now loops (with a timeout and calls to `platform_do_loop_one()`) until *both* `(ps->flags & PROG_FLAG_CDC_TX_BUSY)` is false AND `platform_usart_cdc_tx_busy()` is false. This ensures any previous print operation, regardless of how it was initiated, is fully completed.
    2.  **Flag Ownership:** `debug_printf` sets `ps->flags |= PROG_FLAG_CDC_TX_BUSY;` *before* calling `platform_usart_cdc_tx_async()`.
    3.  **Post-Transmission Wait & Flag Clearing:**
        *   If `platform_usart_cdc_tx_async()` returns `true` (transmission successfully started), `debug_printf` then enters *another* internal loop that waits (with timeout and `platform_do_loop_one()`) only for `platform_usart_cdc_tx_busy()` to become false. This ensures that the specific data it just launched is fully out of the hardware.
        *   After this hardware wait (or its timeout), `debug_printf` clears `ps->flags &= ~PROG_FLAG_CDC_TX_BUSY;`. This makes the function responsible for its entire lifecycle of the busy flag.
        *   If `platform_usart_cdc_tx_async()` returns `false` (failed to start), `ps->flags &= ~PROG_FLAG_CDC_TX_BUSY;` is cleared immediately, as no hardware operation is pending.
-   **Simplification of Helper Functions:** `debug_print_hex` and `debug_print_gps_raw_data` were already refactored in a previous (accepted) step to simply call `debug_printf` for each line/segment of their output. This design relies on `debug_printf` being robustly serializing, which the current changes to `debug_printf` aim to achieve.

### Testing & Validation
-   After this refactoring of `debug_printf`, the output showed that all PM-related debug messages, including the multi-line hexdump generated by `debug_print_hex` (which calls `debug_printf` multiple times), are printing correctly and in sequence.
-   The specific issue of raw GPS data not printing is still present, but this is now likely isolated to the GPS data acquisition or the logic *before* `debug_print_gps_raw_data` is called, rather than the UART printing mechanism itself.

### Lessons Learned
-   When dealing with shared hardware resources like UART managed by asynchronous functions and software flags, ensuring true serialization is critical. A single function (like `debug_printf`) should ideally be the sole gatekeeper for transmissions, managing both hardware and software busy states for its complete operation.
-   Making a low-level print primitive effectively synchronous (blocking until its own hardware operation completes) can simplify higher-level functions that need to print multiple segments, as they no longer need to manage complex inter-segment UART state themselves.
-   If `platform_usart_cdc_tx_busy()` only indicates a shallow hardware state (like DRE) and not the completion of a full buffer transfer by an async call, relying on it alone to clear a software busy flag for the entire operation is a race condition. The software flag must persist until the specific async operation initiated is confirmed complete by the hardware used for that operation.

--- 