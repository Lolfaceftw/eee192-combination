/**
 * @file main.c
 * @brief Module 5 Sample: "Keystroke Hexdump" -> Modified for Plantower PMS Sensor
 *
 * @author Alberto de Villa <alberto.de.villa@eee.upd.edu.ph>
 * @date 28 Oct 2024
 * @modified 17 May 2024 by AI for PMS integration
 */

// Common include for the XC32 compiler
#include <xc.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h> 

#include "platform.h"
#include "pms_parser.h" 

/////////////////////////////////////////////////////////////////////////////

// --- Global Debug Mode Flag ---
// Set to true to enable all debug_printf calls, false to disable them.
static const bool PMS_DEBUG_MODE = false; // CHANGE THIS TO false FOR RELEASE/NORMAL OPERATION

/////////////////////////////////////////////////////////////////////////////
static const char banner_msg[] =
"\033[0m\033[2J\033[1;1H"
"+--------------------------------------------------------------------+\r\n"
"| EEE 192: Electrical and Electronics Engineering Laboratory VI      |\r\n"
"|          Academic Year 2024-2025, Semester 2                       |\r\n"
"|                                                                    |\r\n"
"| Sensor: Plantower PMSxxxx Module with On-Device Parsing (Binary)   |\r\n"
"|                                                                    |\r\n"
"| Author:  Estrada (Supplemented by EEE 158/192)                     |\r\n"
"| Date:    2025                                                      |\r\n"
"+--------------------------------------------------------------------+\r\n"
"\r\n";

//////////////////////////////////////////////////////////////////////////////
typedef struct prog_state_type
{
#define PROG_FLAG_BANNER_PENDING        0x0001	
#define PROG_FLAG_PM_UART_DATA_RECEIVED	0x0004	
#define PROG_FLAG_PMS_DATA_READY_TO_SEND 0x0008 
    
	uint16_t flags;
	
	platform_usart_tx_bufdesc_t tx_desc[4]; 
	char tx_buf[128]; 
	uint16_t tx_blen;
	
	platform_usart_rx_async_desc_t rx_desc;
	uint16_t rx_desc_blen; 
	char rx_desc_buf[32];
    
    platform_usart_rx_async_desc_t pm_rx_desc;
    uint16_t pm_rx_desc_blen; 
    char pm_rx_desc_buf[PMS_PACKET_MAX_LENGTH + 10]; 

    pms_parser_internal_state_t pms_parser_state;
    pms_data_t latest_pms_data; 

    bool debug_tx_busy_for_main_data; 

} prog_state_t;

// --- Debug Print Function ---
void debug_printf(prog_state_t *ps, const char *format, ...) {
    if (!PMS_DEBUG_MODE) { // Check the global debug mode flag
        return;
    }

    if (ps->debug_tx_busy_for_main_data || (ps->flags & PROG_FLAG_BANNER_PENDING)) {
       return;
    }
    
    int G_uart_busy_timeout = 20000; 
    while(platform_usart_cdc_tx_busy() && G_uart_busy_timeout-- > 0) {
        platform_do_loop_one(); 
    }

    if (platform_usart_cdc_tx_busy()) { 
        return; 
    }

    va_list args;
    va_start(args, format);
    int len = vsnprintf(ps->tx_buf, sizeof(ps->tx_buf), format, args);
    va_end(args);

    if (len > 0 && (size_t)len < sizeof(ps->tx_buf)) {
        ps->tx_desc[0].buf = ps->tx_buf; 
        ps->tx_desc[0].len = len;
        
        if(platform_usart_cdc_tx_async(&ps->tx_desc[0], 1)) {
            G_uart_busy_timeout = 20000; 
            while(platform_usart_cdc_tx_busy() && G_uart_busy_timeout-- > 0) {
                platform_do_loop_one(); 
            }
        } 
    }
}

static void prog_setup(prog_state_t *ps)
{
	memset(ps, 0, sizeof(*ps));
	
	platform_init();
	pms_parser_init(&ps->pms_parser_state); 
    ps->debug_tx_busy_for_main_data = false;
	
	ps->rx_desc.buf     = ps->rx_desc_buf;
	ps->rx_desc.max_len = sizeof(ps->rx_desc_buf);
	platform_usart_cdc_rx_async(&ps->rx_desc); 
    
    ps->pm_rx_desc.buf = ps->pm_rx_desc_buf;
    ps->pm_rx_desc.max_len = sizeof(ps->pm_rx_desc_buf);
    pm_platform_usart_cdc_rx_async(&ps->pm_rx_desc); 

    ps->flags |= PROG_FLAG_BANNER_PENDING; 
    debug_printf(ps, "Main: prog_setup() complete. Expecting BINARY PMS data. DEBUG_MODE: %s\r\n", PMS_DEBUG_MODE ? "ON" : "OFF");
	return;
}

static void prog_loop_one(prog_state_t *ps)
{
	uint16_t a = 0;
	
	platform_do_loop_one(); 
	
	if ((a = platform_pb_get_event()) != 0) {
		if ((a & PLATFORM_PB_ONBOARD_PRESS) != 0) {
			ps->flags |= PROG_FLAG_BANNER_PENDING;
            debug_printf(ps, "Main: Banner pending due to PB press.\r\n");
		}
		a = 0;
	}
	
	if ((ps->flags & PROG_FLAG_BANNER_PENDING) != 0) {
        if (!platform_usart_cdc_tx_busy() && !ps->debug_tx_busy_for_main_data) { 
            ps->tx_desc[0].buf = banner_msg;
            ps->tx_desc[0].len = sizeof(banner_msg) - 1;
            if (platform_usart_cdc_tx_async(&ps->tx_desc[0], 1)) {
                ps->flags &= ~PROG_FLAG_BANNER_PENDING;
            } 
        }
    }
	
	if (ps->pm_rx_desc.compl_type == PLATFORM_USART_RX_COMPL_DATA) {
        PORT_SEC_REGS->GROUP[0].PORT_OUTSET = (1 << 15); 
        ps->flags |= PROG_FLAG_PM_UART_DATA_RECEIVED;    
        ps->pm_rx_desc_blen = ps->pm_rx_desc.compl_info.data_len;
        ps->pm_rx_desc.compl_type = PLATFORM_USART_RX_COMPL_NONE; 
    }
    
    if ((ps->flags & PROG_FLAG_PM_UART_DATA_RECEIVED) != 0) {
        bool pms_data_parsed_this_cycle = false;
        
        if (PMS_DEBUG_MODE) { // Only prepare and print hex dump if debug mode is on
            char hex_dump_str[PMS_PACKET_MAX_LENGTH * 3 + 50]; 
            int current_dump_len = 0;
            current_dump_len += snprintf(hex_dump_str + current_dump_len, sizeof(hex_dump_str) - current_dump_len,
                                        "\r\nRX_CHUNK (%u bytes): ", ps->pm_rx_desc_blen);
            for (uint16_t k = 0; k < ps->pm_rx_desc_blen; ++k) {
                if (current_dump_len < (int)sizeof(hex_dump_str) - 4) {
                    current_dump_len += snprintf(hex_dump_str + current_dump_len, sizeof(hex_dump_str) - current_dump_len,
                                                "%02X ", (uint8_t)ps->pm_rx_desc_buf[k]);
                } else {
                    break; 
                }
            }
            if (current_dump_len > 0) {
                debug_printf(ps, "%s\r\n", hex_dump_str); 
            }
        }

        for (uint16_t i = 0; i < ps->pm_rx_desc_blen; ++i) {
            uint8_t byte_from_pms = (uint8_t)ps->pm_rx_desc_buf[i]; 
            pms_parser_status_t status = pms_parser_feed_byte(ps, &ps->pms_parser_state, byte_from_pms, &ps->latest_pms_data);

            if (status == PMS_PARSER_OK) {
                debug_printf(ps, "Main: PMS_PARSER_OK! Data available.\r\n");
                
                ps->debug_tx_busy_for_main_data = true; 
                ps->tx_blen = snprintf(ps->tx_buf, sizeof(ps->tx_buf),
                                       "PMS Data -> PM1.0: %u, PM2.5: %u, PM10: %u (ug/m3 ATM)\r\n",
                                       ps->latest_pms_data.pm1_0_atm,
                                       ps->latest_pms_data.pm2_5_atm,
                                       ps->latest_pms_data.pm10_atm);
                
                if (ps->tx_blen > 0 && ps->tx_blen < sizeof(ps->tx_buf)) {
                    ps->flags |= PROG_FLAG_PMS_DATA_READY_TO_SEND;
                    pms_data_parsed_this_cycle = true; 
                } else {
                    debug_printf(ps, "Main ERR: snprintf failed for parsed data.\r\n");
                    ps->debug_tx_busy_for_main_data = false; 
                }
            } 
            // No need to explicitly call debug_printf for parser errors here,
            // as pms_parser_feed_byte itself calls debug_printf internally,
            // and those calls will be controlled by PMS_DEBUG_MODE.
        }
        
        ps->flags &= ~PROG_FLAG_PM_UART_DATA_RECEIVED; 

        if (!pms_data_parsed_this_cycle) { 
             pm_platform_usart_cdc_rx_async(&ps->pm_rx_desc); 
        }
         PORT_SEC_REGS->GROUP[0].PORT_OUTCLR = (1 << 15); 
    }
    
    if ((ps->flags & PROG_FLAG_PMS_DATA_READY_TO_SEND) != 0) {
        int G_main_data_wait_timeout = 20000;
        while(platform_usart_cdc_tx_busy() && G_main_data_wait_timeout-- > 0) {
            platform_do_loop_one();
        }

        if (platform_usart_cdc_tx_busy()) {
            // If PMS_DEBUG_MODE is true, this message will print. Otherwise, it won't.
            debug_printf(ps, "Main ERR: UART still busy before sending main PMS data.\r\n");
        } else {
            ps->tx_desc[0].buf = ps->tx_buf;
            ps->tx_desc[0].len = ps->tx_blen;

            if (platform_usart_cdc_tx_async(&ps->tx_desc[0], 1)) {
                ps->flags &= ~PROG_FLAG_PMS_DATA_READY_TO_SEND;
                ps->debug_tx_busy_for_main_data = false; 
                pm_platform_usart_cdc_rx_async(&ps->pm_rx_desc); 
            } else {
                debug_printf(ps, "Main ERR: Parsed PMS data TX async FAILED.\r\n");
            }
        }
    }
	return;
}

int main(void)
{
	prog_state_t ps_instance; 
	
	prog_setup(&ps_instance);
	
	for (;;) {
		prog_loop_one(&ps_instance);
	}
    
    return 1; 
}