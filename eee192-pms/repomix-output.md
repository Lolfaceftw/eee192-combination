This file is a merged representation of the entire codebase, combined into a single document by Repomix.

# File Summary

## Purpose
This file contains a packed representation of the entire repository's contents.
It is designed to be easily consumable by AI systems for analysis, code review,
or other automated processes.

## File Format
The content is organized as follows:
1. This summary section
2. Repository information
3. Directory structure
4. Repository files (if enabled)
4. Multiple file entries, each consisting of:
  a. A header with the file path (## File: path/to/file)
  b. The full contents of the file in a code block

## Usage Guidelines
- This file should be treated as read-only. Any changes should be made to the
  original repository files, not this packed version.
- When processing this file, use the file path to distinguish
  between different files in the repository.
- Be aware that this file may contain sensitive information. Handle it with
  the same level of security as you would the original repository.

## Notes
- Some files may have been excluded based on .gitignore rules and Repomix's configuration
- Binary files are not included in this packed representation. Please refer to the Repository Structure section for a complete list of file paths, including binary files
- Files matching patterns in .gitignore are excluded
- Files matching default ignore patterns are excluded
- Files are sorted by Git change count (files with more changes are at the bottom)

## Additional Info

# Directory Structure
```
.repomixignore
convert.py
main.c
Makefile
platform.h
platform/gpio.c
platform/pm_usart.c
platform/systick.c
platform/usart.c
repomix.config.json
```

# Files

## File: .repomixignore
```
# Add patterns to ignore here, one per line
# Example:
# *.log
# tmp/
.generated_files/*
build/*
debug/*
dist/*
gps/*
nbproject/*
```

## File: repomix.config.json
```json
{
  "input": {
    "maxFileSize": 52428800
  },
  "output": {
    "filePath": "repomix-output.md",
    "style": "markdown",
    "parsableStyle": false,
    "fileSummary": true,
    "directoryStructure": true,
    "files": true,
    "removeComments": false,
    "removeEmptyLines": false,
    "compress": false,
    "topFilesLength": 5,
    "showLineNumbers": false,
    "copyToClipboard": false,
    "git": {
      "sortByChanges": true,
      "sortByChangesMaxCommits": 100
    }
  },
  "include": [],
  "ignore": {
    "useGitignore": true,
    "useDefaultPatterns": true,
    "customPatterns": []
  },
  "security": {
    "enableSecurityCheck": true
  },
  "tokenCount": {
    "encoding": "o200k_base"
  }
}
```

## File: main.c
```cpp
/**
 * @file main.c
 * @brief Module 5 Sample: "Keystroke Hexdump"
 *
 * @author Alberto de Villa <alberto.de.villa@eee.upd.edu.ph>
 * @date 28 Oct 2024
 */

// Common include for the XC32 compiler
#include <xc.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "platform.h"

/////////////////////////////////////////////////////////////////////////////

/*
 * Copyright message printed upon reset
 * 
 * Displaying author information is optional; but as always, must be present
 * as comments at the top of the source file for copyright purposes.
 * 
 * FIXME: Modify this prompt message to account for additional instructions.
 */
static const char banner_msg[] =
"\033[0m\033[2J\033[1;1H"
"+--------------------------------------------------------------------+\r\n"
"| EEE 192: Electrical and Electronics Engineering Laboratory VI      |\r\n"
"|          Academic Year 2024-2025, Semester 2                       |\r\n"
"|                                                                    |\r\n"
"| Sensor: PM Module                                                  |\r\n"
"|                                                                    |\r\n"
"| Author:  Estrada (Supplemented by EEE 158 AY 24-25 1S)             |\r\n"
"| Date:    2025                                                      |\r\n"
"+--------------------------------------------------------------------+\r\n"
"\r\n"
"Data: ";

static const char ESC_SEQ_KEYP_LINE[] = "\033[12;1H\033[0K";
static const char ESC_SEQ_IDLE_INF[]  = "\033[20;1H";

//////////////////////////////////////////////////////////////////////////////

// Program state machine
typedef struct prog_state_type
{
	// Flags for this program
#define PROG_FLAG_BANNER_PENDING        0x0001	// Waiting to transmit the banner
#define PROG_FLAG_UPDATE_PENDING        0x0002	// Waiting to transmit updates
#define PROG_FLAG_pm_UPDATE_PENDING	0x0004	// Waiting to transmit updates
#define PROG_FLAG_GEN_COMPLETE      0x8000	// Message generation has been done, but transmission has not occurred
    
	uint16_t flags;
	
	// Transmit stuff
	platform_usart_tx_bufdesc_t tx_desc[4];
	char tx_buf[64];
	uint16_t tx_blen;
	
	// Receiver stuff
	platform_usart_rx_async_desc_t rx_desc;
	uint16_t rx_desc_blen;
	char rx_desc_buf[32];
    
    // Receive from pm
    platform_usart_rx_async_desc_t pm_rx_desc;
    uint16_t pm_rx_desc_blen;
    char pm_rx_desc_buf[64];
    
} prog_state_t;

/*
 * Initialize the main program state
 * 
 * This style might be familiar to those accustomed to he programming
 * conventions employed by the Arduino platform.
 */
static void prog_setup(prog_state_t *ps)
{
	memset(ps, 0, sizeof(*ps));
	
	platform_init();
	
    // SERCOM3 - Keyb + PIC32
    
	ps->rx_desc.buf     = ps->rx_desc_buf;
	ps->rx_desc.max_len = sizeof(ps->rx_desc_buf);
	
	platform_usart_cdc_rx_async(&ps->rx_desc);
    
    // SERCOM1 - pm

    ps->pm_rx_desc.buf = ps->pm_rx_desc_buf;
    ps->pm_rx_desc.max_len = sizeof(ps->pm_rx_desc_buf);
    
    pm_platform_usart_cdc_rx_async(&ps->pm_rx_desc);
	return;
}


/*
 * Do a single loop of the main program
 * 
 * This style might be familiar to those accustomed to he programming
 * conventions employed by the Arduino platform.
 */
static void prog_loop_one(prog_state_t *ps)
{
	uint16_t a = 0;
	
	// Do one iteration of the platform event loop first.
	platform_do_loop_one();
	
	// Something happened to the pushbutton?
	if ((a = platform_pb_get_event()) != 0) {
		if ((a & PLATFORM_PB_ONBOARD_PRESS) != 0) {
			// Print out the banner
			ps->flags |= PROG_FLAG_BANNER_PENDING;
		}
		a = 0;
	}
	
	////////////////////////////////////////////////////////////////////
	
	// Process any pending flags (BANNER)
	do {
		if ((ps->flags & PROG_FLAG_BANNER_PENDING) == 0)
			break;
		
		if (platform_usart_cdc_tx_busy())
			break;
		
		if ((ps->flags & PROG_FLAG_GEN_COMPLETE) == 0) {
			// Message has not been generated.
			ps->tx_desc[0].buf = banner_msg;
			ps->tx_desc[0].len = sizeof(banner_msg)-1;
			ps->flags |= PROG_FLAG_GEN_COMPLETE;
		}
		
		if (platform_usart_cdc_tx_async(&ps->tx_desc[0], 1)) {
			ps->flags &= ~(PROG_FLAG_BANNER_PENDING | PROG_FLAG_GEN_COMPLETE);
		}
	} while (0);
	
    // Something from the SERCOM0 UART?
	if (ps->pm_rx_desc.compl_type == PLATFORM_USART_RX_COMPL_DATA) {
        PORT_SEC_REGS->GROUP[0].PORT_OUTSET = (1 << 15);
        ps->flags |= PROG_FLAG_pm_UPDATE_PENDING;
        ps->pm_rx_desc_blen = ps->pm_rx_desc.compl_info.data_len;
    }
    
    // Process any pending pm update flags
    do {
		if ((ps->flags & PROG_FLAG_pm_UPDATE_PENDING) == 0)
			break;
		
		if (platform_usart_cdc_tx_busy())
			break;
		
		if ((ps->flags & PROG_FLAG_GEN_COMPLETE) == 0) {
            PORT_SEC_REGS->GROUP[0].PORT_OUTCLR = (1 << 15);
            
            
			ps->tx_desc[0].buf = ps->pm_rx_desc_buf;
			ps->tx_desc[0].len = ps->pm_rx_desc_blen;
		}
		
		if (platform_usart_cdc_tx_async(&ps->tx_desc, 1)) {
			ps->pm_rx_desc.compl_type = PLATFORM_USART_RX_COMPL_NONE;
			pm_platform_usart_cdc_rx_async(&ps->pm_rx_desc);
			ps->flags &= ~(PROG_FLAG_pm_UPDATE_PENDING | PROG_FLAG_GEN_COMPLETE);
		}
	} while (0);
	
	// Done
	return;
}

// main() -- the heart of the program
int main(void)
{
	prog_state_t ps;
	
	// Initialization time	
	prog_setup(&ps);
	
	/*
	 * Microcontroller main()'s are supposed to never return (welp, they
	 * have none to return to); hence the intentional infinite loop.
	 */
	for (;;) {
		prog_loop_one(&ps);
	}
    
    // This line must never be reached
    return 1;
}
```

## File: Makefile
```
#
#  There exist several targets which are by default empty and which can be 
#  used for execution of your targets. These targets are usually executed 
#  before and after some main targets. They are: 
#
#     .build-pre:              called before 'build' target
#     .build-post:             called after 'build' target
#     .clean-pre:              called before 'clean' target
#     .clean-post:             called after 'clean' target
#     .clobber-pre:            called before 'clobber' target
#     .clobber-post:           called after 'clobber' target
#     .all-pre:                called before 'all' target
#     .all-post:               called after 'all' target
#     .help-pre:               called before 'help' target
#     .help-post:              called after 'help' target
#
#  Targets beginning with '.' are not intended to be called on their own.
#
#  Main targets can be executed directly, and they are:
#  
#     build                    build a specific configuration
#     clean                    remove built files from a configuration
#     clobber                  remove all built files
#     all                      build all configurations
#     help                     print help mesage
#  
#  Targets .build-impl, .clean-impl, .clobber-impl, .all-impl, and
#  .help-impl are implemented in nbproject/makefile-impl.mk.
#
#  Available make variables:
#
#     CND_BASEDIR                base directory for relative paths
#     CND_DISTDIR                default top distribution directory (build artifacts)
#     CND_BUILDDIR               default top build directory (object files, ...)
#     CONF                       name of current configuration
#     CND_ARTIFACT_DIR_${CONF}   directory of build artifact (current configuration)
#     CND_ARTIFACT_NAME_${CONF}  name of build artifact (current configuration)
#     CND_ARTIFACT_PATH_${CONF}  path to build artifact (current configuration)
#     CND_PACKAGE_DIR_${CONF}    directory of package (current configuration)
#     CND_PACKAGE_NAME_${CONF}   name of package (current configuration)
#     CND_PACKAGE_PATH_${CONF}   path to package (current configuration)
#
# NOCDDL


# Environment 
MKDIR=mkdir
CP=cp
CCADMIN=CCadmin
RANLIB=ranlib


# build
build: .build-post

.build-pre:
# Add your pre 'build' code here...

.build-post: .build-impl
# Add your post 'build' code here...


# clean
clean: .clean-post

.clean-pre:
# Add your pre 'clean' code here...
# WARNING: the IDE does not call this target since it takes a long time to
# simply run make. Instead, the IDE removes the configuration directories
# under build and dist directly without calling make.
# This target is left here so people can do a clean when running a clean
# outside the IDE.

.clean-post: .clean-impl
# Add your post 'clean' code here...


# clobber
clobber: .clobber-post

.clobber-pre:
# Add your pre 'clobber' code here...

.clobber-post: .clobber-impl
# Add your post 'clobber' code here...


# all
all: .all-post

.all-pre:
# Add your pre 'all' code here...

.all-post: .all-impl
# Add your post 'all' code here...


# help
help: .help-post

.help-pre:
# Add your pre 'help' code here...

.help-post: .help-impl
# Add your post 'help' code here...



# include project implementation makefile
include nbproject/Makefile-impl.mk

# include project make variables
include nbproject/Makefile-variables.mk
```

## File: platform.h
```
/**
 * @file  platform.h
 * @brief Declarations for platform-support routines
 *
 * @author Alberto de Villa <alberto.de.villa@eee.upd.edu.ph>
 * @date   28 Oct 2024
 */

#if !defined(EEE158_EX05_PLATFORM_H_) 
#define EEE158_EX05_PLATFORM_H_

#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include <stdint.h>

// C linkage should be maintained
#ifdef __cplusplus
extern "C" {
#endif

/// Initialize the platform, including any hardware peripherals.
void platform_init(void);

/**
 * Do one loop of events processing for the platform
 * 
 * @note
 * This is expected to be called within the main application infinite loop.
 */
void platform_do_loop_one(void);

//////////////////////////////////////////////////////////////////////////////

/// Pushbutton event mask for pressing the on-board button
#define PLATFORM_PB_ONBOARD_PRESS	0x0001

/// Pushbutton event mask for releasing the on-board button
#define PLATFORM_PB_ONBOARD_RELEASE	0x0002

/// Pushbutton event mask for the on-board button
#define PLATFORM_PB_ONBOARD_MASK	(PLATFORM_PB_ONBOARD_PRESS | PLATFORM_PB_ONBOARD_RELEASE)

/**
 * Determine which pushbutton events have pressed since this function was last
 * called
 * 
 * @return	A bitmask of @code PLATFORM_PB_* @endcode values denoting which
 * 	        event/s occurred
 */
uint16_t platform_pb_get_event(void);

//////////////////////////////////////////////////////////////////////////////

/// GPO flag for the onboard LED
#define PLATFORM_GPO_LED_ONBOARD	0x0001

/**
 * Modify the GP output state/s according to the provided bitmask/s
 * 
 * @param[in]	set	LED/s to turn ON; set to zero if unused
 * @param[in]	clr	LED/s to turn OFF; set to zero if unused
 * 
 * @note
 * CLEAR overrides SET, if the same GP output exists in both parameters.
 */
void platform_gpo_modify(uint16_t set, uint16_t clr);

//////////////////////////////////////////////////////////////////////////////

/**
 * Structure representing a time specification
 * 
 * @note
 * This is inspired by @code struct timespec @endcode used within the Linux
 * kernel and syscall APIs, but is not intended to be compatible with either
 * set of APIs.
 */
typedef struct platform_timespec_type {
	/// Number of seconds elapsed since some epoch
	uint32_t	nr_sec;
	
	/**
	 * Number of nanoseconds
	 * 
	 * @note
	 * Routines expect this value to lie on the interval [0, 999999999].
	 */
	uint32_t	nr_nsec;
} platform_timespec_t;

/// Initialize a @c timespec structure to zero
#define PLATFORM_TIMESPEC_ZERO {0, 0}

/**
 * Compare two timespec instances
 * 
 * @param[in]	lhs	Left-hand side
 * @param[in]	rhs	Right-hand side
 * 
 * @return -1 if @c lhs is earlier than @c rhs, +1 if @c lhs is later than
 *         @c rhs, zero otherwise
 */
int platform_timespec_compare(const platform_timespec_t *lhs,
	const platform_timespec_t *rhs);

/// Number of microseconds for a single tick
#define	PLATFORM_TICK_PERIOD_US	5000

/// Return the number of ticks since @c platform_init() was called
void platform_tick_count(platform_timespec_t *tick);

/**
 * A higher-resolution version of @c platform_tick_count(), if available
 * 
 * @note
 * If unavailable, this function is equivalent to @c platform_tick_count().
 */
void platform_tick_hrcount(platform_timespec_t *tick);

/**
 * Get the difference between two ticks
 * 
 * @note
 * This routine accounts for wrap-arounds, but only once.
 * 
 * @param[out]	diff	Difference
 * @param[in]	lhs	Left-hand side
 * @param[in]	rhs	Right-hand side
 */
void platform_tick_delta(
	platform_timespec_t *diff,
	const platform_timespec_t *lhs, const platform_timespec_t *rhs
	);

//////////////////////////////////////////////////////////////////////////////

/// Descriptor for reception via USART
typedef struct platform_usart_rx_desc_type
{
	/// Buffer to store received data into
	char *buf;
	
	/// Maximum number of bytes for @c buf
	uint16_t max_len;
	
	/// Type of completion that has occurred
	volatile uint16_t compl_type;
	
/// No reception-completion event has occurred
#define PLATFORM_USART_RX_COMPL_NONE	0x0000

/// Reception completed with a received packet
#define PLATFORM_USART_RX_COMPL_DATA	0x0001

/**
 * Reception completed with a line break
 * 
 * @note
 * This completion event is not implemented in this sample.
 */
#define PLATFORM_USART_RX_COMPL_BREAK	0x0002

	/// Extra information about a completion event, if applicable
	volatile union {
		/**
		 * Number of bytes that were received
		 * 
		 * @note
		 * This member is valid only if @code compl_type == PLATFORM_USART_RX_COMPL_DATA @endcode.
		 */
		uint16_t data_len;
	} compl_info;
} platform_usart_rx_async_desc_t;

/// Descriptor for a transmission fragment
typedef struct platform_usart_tx_desc_type
{
	/// Start of the buffer to transmit
	const char *buf;
	
	/// Size of the buffer
	uint16_t len;
} platform_usart_tx_bufdesc_t;

/**
 * Enqueue an array of fragments for transmission
 * 
 * @note
 * All fragment-array elements and source buffer/s must remain valid for the
 * entire time transmission is on-going.
 * 
 * @p	desc	Descriptor array
 * @p	nr_desc	Number of descriptors
 * 
 * @return	@c true if the transmission is successfully enqueued, @c false
 *		otherwise
 */
bool platform_usart_cdc_tx_async(const platform_usart_tx_bufdesc_t *desc,
				 unsigned int nr_desc);

/// Abort an ongoing transmission
void platform_usart_cdc_tx_abort(void);

/// Check whether a transmission is on-going
bool platform_usart_cdc_tx_busy(void);

/**
 * Enqueue a request for data reception
 * 
 * @note
 * Both descriptor and target buffer must remain valid for the entire time
 * reception is on-going.
 * 
 * @p	desc	Descriptor
 * 
 * @return	@c true if the reception is successfully enqueued, @c false
 *		otherwise
 */
bool platform_usart_cdc_rx_async(platform_usart_rx_async_desc_t *desc);

/// Abort an ongoing transmission
void platform_usart_cdc_rx_abort(void);

/// Check whether a reception is on-going
bool platform_usart_cdc_rx_busy(void);

//////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif	// __cplusplus
#endif	// !defined(EEE158_EX05_PLATFORM_H_)
```

## File: platform/gpio.c
```cpp
/**
 * @file platform/gpio.c
 * @brief Platform-support routines, GPIO component + initialization entrypoints
 *
 * @author Alberto de Villa <alberto.de.villa@eee.upd.edu.ph>
 * @date   28 Oct 2024
 */

/*
 * PIC32CM5164LS00048 initial configuration:
 * -- Architecture: ARMv8 Cortex-M23
 * -- GCLK_GEN0: OSC16M @ 4 MHz, no additional prescaler
 * -- Main Clock: No additional prescaling (always uses GCLK_GEN0 as input)
 * -- Mode: Secure, NONSEC disabled
 * 
 * New clock configuration:
 * -- GCLK_GEN0: 24 MHz (DFLL48M [48 MHz], with /2 prescaler)
 * -- GCLK_GEN2: 4 MHz  (OSC16M @ 4 MHz, no additional prescaler)
 * 
 * HW configuration for the corresponding Curiosity Nano+ Touch Evaluation
 * Board:
 * -- PA15: Active-HI LED
 * -- PA23: Active-LO PB w/ external pull-up
 */

// Common include for the XC32 compiler
#include <xc.h>
#include <stdbool.h>
#include <string.h>

#include "../platform.h"

// Initializers defined in other platform/*.c files
extern void platform_systick_init(void);

extern void platform_usart_init(void);
extern void platform_usart_tick_handler(const platform_timespec_t *tick);

extern void pm_platform_usart_init(void);
extern void pm_platform_usart_tick_handler(const platform_timespec_t *tick);

/////////////////////////////////////////////////////////////////////////////

// Enable higher frequencies for higher performance
static void raise_perf_level(void)
{
	uint32_t tmp_reg = 0;
	
	/*
	 * The chip starts in PL0, which emphasizes energy efficiency over
	 * performance. However, we need the latter for the clock frequency
	 * we will be using (~24 MHz); hence, switch to PL2 before continuing.
	 */
	PM_REGS->PM_INTFLAG = 0x01;
	PM_REGS->PM_PLCFG = 0x02;
	while ((PM_REGS->PM_INTFLAG & 0x01) == 0)
		asm("nop");
	PM_REGS->PM_INTFLAG = 0x01;
	
	/*
	 * Power up the 48MHz DFPLL.
	 * 
	 * On the Curiosity Nano Board, VDDPLL has a 1.1uF capacitance
	 * connected in parallel. Assuming a ~20% error, we have
	 * STARTUP >= (1.32uF)/(1uF) = 1.32; as this is not an integer, choose
	 * the next HIGHER value.
	 */
	NVMCTRL_SEC_REGS->NVMCTRL_CTRLB = (2 << 1) ;
	SUPC_REGS->SUPC_VREGPLL = 0x00000302;
	while ((SUPC_REGS->SUPC_STATUS & (1 << 18)) == 0)
		asm("nop");
	
	/*
	 * Configure the 48MHz DFPLL.
	 * 
	 * Start with disabling ONDEMAND...
	 */
	OSCCTRL_REGS->OSCCTRL_DFLLCTRL = 0x0000;
	while ((OSCCTRL_REGS->OSCCTRL_STATUS & (1 << 24)) == 0)
		asm("nop");
	
	/*
	 * ... then writing the calibration values (which MUST be done as a
	 * single write, hence the use of a temporary variable)...
	 */
	tmp_reg  = *((uint32_t*)0x00806020);
	tmp_reg &= ((uint32_t)(0b111111) << 25);
	tmp_reg >>= 15;
	tmp_reg |= ((512 << 0) & 0x000003ff);
	OSCCTRL_REGS->OSCCTRL_DFLLVAL = tmp_reg;
	while ((OSCCTRL_REGS->OSCCTRL_STATUS & (1 << 24)) == 0)
		asm("nop");
	
	// ... then enabling ...
	OSCCTRL_REGS->OSCCTRL_DFLLCTRL |= 0x0002;
	while ((OSCCTRL_REGS->OSCCTRL_STATUS & (1 << 24)) == 0)
		asm("nop");
	
	// ... then restoring ONDEMAND.
//	OSCCTRL_REGS->OSCCTRL_DFLLCTRL |= 0x0080;
//	while ((OSCCTRL_REGS->OSCCTRL_STATUS & (1 << 24)) == 0)
//		asm("nop");
	
	/*
	 * Configure GCLK_GEN2 as described; this one will become the main
	 * clock for slow/medium-speed peripherals, as GCLK_GEN0 will be
	 * stepped up for 24 MHz operation.
	 */
	GCLK_REGS->GCLK_GENCTRL[2] = 0x00000105;
	while ((GCLK_REGS->GCLK_SYNCBUSY & (1 << 4)) != 0)
		asm("nop");
	
	// Switch over GCLK_GEN0 to DFLL48M, with DIV=2 to get 24 MHz.
	GCLK_REGS->GCLK_GENCTRL[0] = 0x00020107;
	while ((GCLK_REGS->GCLK_SYNCBUSY & (1 << 2)) != 0)
		asm("nop");
	
	// Done. We're now at 24 MHz.
	return;
}

/*
 * Configure the EIC peripheral
 * 
 * NOTE: EIC initialization is split into "early" and "late" halves. This is
 *       because most settings within the peripheral cannot be modified while
 *       EIC is enabled.
 */
static void EIC_init_early(void)
{
	/*
	 * Enable the APB clock for this peripheral
	 * 
	 * NOTE: The chip resets with it enabled; hence, commented-out.
	 * 
	 * WARNING: Incorrect MCLK settings can cause system lockup that can
	 *          only be rectified via a hardware reset/power-cycle.
	 */
	// MCLK_REGS->MCLK_APBAMASK |= (1 << 10);
	
	/*
	 * In order for debouncing to work, GCLK_EIC needs to be configured.
	 * We can pluck this off GCLK_GEN2, configured for 4 MHz; then, for
	 * mechanical inputs we slow it down to around 15.625 kHz. This
	 * prescaling is OK for such inputs since debouncing is only employed
	 * on inputs connected to mechanical switches, not on those coming from
	 * other (electronic) circuits.
	 * 
	 * GCLK_EIC is at index 4; and Generator 2 is used.
	 */
	GCLK_REGS->GCLK_PCHCTRL[4] = 0x00000042;
	while ((GCLK_REGS->GCLK_PCHCTRL[4] & 0x00000042) == 0)
		asm("nop");
	
	// Reset, and wait for said operation to complete.
	EIC_SEC_REGS->EIC_CTRLA = 0x01;
	while ((EIC_SEC_REGS->EIC_SYNCBUSY & 0x01) != 0)
		asm("nop");
	
	/*
	 * Just set the debounce prescaler for now, and leave the EIC disabled.
	 * This is because most settings are not editable while the peripheral
	 * is enabled.
	 */
	EIC_SEC_REGS->EIC_DPRESCALER = (0b0 << 16) | (0b0000 << 4) |
		                       (0b1111 << 0);
	return;
}
static void EIC_init_late(void)
{
	/*
	 * Enable the peripheral.
	 * 
	 * Once the peripheral is enabled, further configuration is almost
	 * impossible.
	 */
	EIC_SEC_REGS->EIC_CTRLA |= 0x02;
	while ((EIC_SEC_REGS->EIC_SYNCBUSY & 0x02) != 0)
		asm("nop");
	return;
}

// Configure the EVSYS peripheral
static void EVSYS_init(void)
{
	/*
	 * Enable the APB clock for this peripheral
	 * 
	 * NOTE: The chip resets with it enabled; hence, commented-out.
	 * 
	 * WARNING: Incorrect MCLK settings can cause system lockup that can
	 *          only be rectified via a hardware reset/power-cycle.
	 */
	// MCLK_REGS->MCLK_APBAMASK |= (1 << 0);
	
	/*
	 * EVSYS is always enabled, but may be in an inconsistent state. As
	 * such, trigger a reset.
	 */
	EVSYS_SEC_REGS->EVSYS_CTRLA = 0x01;
	asm("nop");
	asm("nop");
	asm("nop");
	return;
}

//////////////////////////////////////////////////////////////////////////////

/*
 * Initialize the general-purpose output
 * 
 * NOTE: PORT I/O configuration is never separable from the in-circuit wiring.
 *       Refer to the top of this source file for each PORT pin assignments.
 */
static void GPO_init(void)
{
	// On-board LED (PA15)
	PORT_SEC_REGS->GROUP[0].PORT_OUTCLR = (1 << 15);
	PORT_SEC_REGS->GROUP[0].PORT_DIRSET = (1 << 15);
	PORT_SEC_REGS->GROUP[0].PORT_PINCFG[15] = 0x00;
	
	// Done
	return;
}
	
// Turn ON the LED
void platform_gpo_modify(uint16_t set, uint16_t clr)
{
	uint32_t p_s[2] = {0, 0};
	uint32_t p_c[2] = {0, 0};
	
	// CLR overrides SET
	set &= ~(clr);
	
	// SET first...
	if ((set & PLATFORM_GPO_LED_ONBOARD) != 0)
		p_s[0] |= (1 << 15);
	
	// ... then CLR.
	if ((clr & PLATFORM_GPO_LED_ONBOARD) != 0)
		p_c[0] |= (1 << 15);
	
	// Commit the changes
	PORT_SEC_REGS->GROUP[0].PORT_OUTSET = p_s[0];
	PORT_SEC_REGS->GROUP[0].PORT_OUTCLR = p_c[0];
	return;
}

//////////////////////////////////////////////////////////////////////////////

/*
 * Per the datasheet for the PIC32CM5164LS00048, PA23 belongs to EXTINT[2],
 * which in turn is Peripheral Function A. The corresponding Interrupt ReQuest
 * (IRQ) handler is thus named EIC_EXTINT_2_Handler.
 */
static volatile uint16_t pb_press_mask = 0;
void __attribute__((used, interrupt())) EIC_EXTINT_2_Handler(void)
{
	pb_press_mask &= ~PLATFORM_PB_ONBOARD_MASK;
	if ((EIC_SEC_REGS->EIC_PINSTATE & (1 << 2)) == 0)
		pb_press_mask |= PLATFORM_PB_ONBOARD_PRESS;
	else
		pb_press_mask |= PLATFORM_PB_ONBOARD_RELEASE;
	
	// Clear the interrupt before returning.
	EIC_SEC_REGS->EIC_INTFLAG |= (1 << 2);
	return;
}
static void PB_init(void)
{
	/*
	 * Configure PA23.
	 * 
	 * NOTE: PORT I/O configuration is never separable from the in-circuit
	 *       wiring. Refer to the top of this source file for each PORT
	 *       pin assignments.
	 */
	PORT_SEC_REGS->GROUP[0].PORT_DIRCLR = 0x00800000;
	PORT_SEC_REGS->GROUP[0].PORT_PINCFG[23] = 0x03;
	PORT_SEC_REGS->GROUP[0].PORT_PMUX[(23 >> 1)] &= ~(0xF0);
	
	/*
	 * Debounce EIC_EXT2, where PA23 is.
	 * 
	 * Configure the line for edge-detection only.
	 * 
	 * NOTE: EIC has been reset and pre-configured by the time this
	 *       function is called.
	 */
	EIC_SEC_REGS->EIC_DEBOUNCEN |= (1 << 2);
	EIC_SEC_REGS->EIC_CONFIG0   &= ~((uint32_t)(0xF) << 8);
	EIC_SEC_REGS->EIC_CONFIG0   |=  ((uint32_t)(0xB) << 8);
	
	/*
	 * NOTE: Even though interrupts are enabled here, global interrupts
	 *       still need to be enabled via NVIC.
	 */
	EIC_SEC_REGS->EIC_INTENSET = 0x00000004;
	return;
}

// Get the mask of currently-pressed buttons
uint16_t platform_pb_get_event(void)
{
	uint16_t cache = pb_press_mask;
	
	pb_press_mask = 0;
	return cache;
}

//////////////////////////////////////////////////////////////////////////////

/*
 * Configure the NVIC
 * 
 * This must be called last, because interrupts are enabled as soon as
 * execution returns from this function.
 */
static void NVIC_init(void)
{
	/*
	 * Unlike AHB/APB peripherals, the NVIC is part of the Arm v8-M
	 * architecture core proper. Hence, it is always enabled.
	 */
	__DMB();
	__enable_irq();
	NVIC_SetPriority(EIC_EXTINT_2_IRQn, 3);
	NVIC_SetPriority(SysTick_IRQn, 3);
	NVIC_EnableIRQ(EIC_EXTINT_2_IRQn);
	NVIC_EnableIRQ(SysTick_IRQn);
	return;
}

/////////////////////////////////////////////////////////////////////////////

// Initialize the platform
void platform_init(void)
{
	// Raise the power level
	raise_perf_level();
	
	// Early initialization
	EVSYS_init();
	EIC_init_early();
	
	// Regular initialization
	PB_init();
	GPO_init();
	platform_usart_init();
    pm_platform_usart_init();
	
	// Late initialization
	EIC_init_late();
	platform_systick_init();
	NVIC_init();
	return;
}

// Do a single event loop
void platform_do_loop_one(void)
{
    
	/*
	 * Some routines must be serviced as quickly as is practicable. Do so
	 * now.
	 */
	platform_timespec_t tick;
	
	platform_tick_hrcount(&tick);
    
	platform_usart_tick_handler(&tick);
	pm_platform_usart_tick_handler(&tick);
}
```

## File: platform/pm_usart.c
```cpp
/**
 * @file platform/usart.c
 * @brief Platform-support routines, USART component
 *
 * @author Alberto de Villa <alberto.de.villa@eee.upd.edu.ph>
 * @date   28 Oct 2024
 */

/*
 * PIC32CM5164LS00048 initial configuration:
 * -- Architecture: ARMv8 Cortex-M23
 * -- GCLK_GEN0: OSC16M @ 4 MHz, no additional prescaler
 * -- Main Clock: No additional prescaling (always uses GCLK_GEN0 as input)
 * -- Mode: Secure, NONSEC disabled
 * 
 * HW configuration for the corresponding Curiosity Nano+ Touch Evaluation
 * Board:
 * -- PB17: UART via debugger (RX, SERCOM1, PAD[1])
 */

// Common include for the XC32 compiler
#include <xc.h>
#include <stdbool.h>
#include <string.h>

#include "../platform.h"

// Functions "exported" by this file
void pm_platform_usart_init(void);
void pm_platform_usart_tick_handler(const platform_timespec_t *tick);

/////////////////////////////////////////////////////////////////////////////

/**
 * State variables for UART
 * 
 * NOTE: Since these are shared between application code and interrupt handlers
 *       (SysTick and SERCOM), these must be declared volatile.
 */
typedef struct ctx_usart_type {
	
	/// Pointer to the underlying register set
	sercom_usart_int_registers_t *regs;
	
	/// State variables for the transmitter
	struct {
		volatile platform_usart_tx_bufdesc_t *desc;
		volatile uint16_t nr_desc;
		
		// Current descriptor
		volatile const char *buf;
		volatile uint16_t    len;
	} tx;
	
	/// State variables for the receiver
	struct {
		/// Receive descriptor, held by the client
		volatile platform_usart_rx_async_desc_t * volatile desc;
		
		/// Tick since the last character was received
		volatile platform_timespec_t ts_idle;
		
		/// Index at which to place an incoming character
		volatile uint16_t idx;
				
	} rx;
	
	/// Configuration items
	struct {
		/// Idle timeout (reception only)
		platform_timespec_t ts_idle_timeout;
	} cfg;
	
} pm_ctx_usart_t;
static pm_ctx_usart_t pm_ctx_uart;

// Configure USART
void pm_platform_usart_init(void){
	/*
	 * For ease of typing, #define a macro corresponding to the SERCOM
	 * peripheral and its internally-clocked USART view.
	 * 
	 * To avoid namespace pollution, this macro is #undef'd at the end of
	 * this function.
	 */
#define UART_REGS (&(SERCOM0_REGS->USART_INT))
	
	/*
	 * Enable the APB clock for this peripheral
	 * 
	 * NOTE: The chip resets with it enabled; hence, commented-out.
	 * 
	 * WARNING: Incorrect MCLK settings can cause system lockup that can
	 *          only be rectified via a hardware reset/power-cycle.
	 */
	// MCLK_REGS->MCLK_APB???MASK |= (1 << ???);
	
	/*
	 * Enable the GCLK generator for this peripheral
	 * 
	 * NOTE: GEN2 (4 MHz) is used, as GEN0 (24 MHz) is too fast for our
	 *       use case.
	 */
	GCLK_REGS->GCLK_PCHCTRL[17] = 0x00000042;
	while ((GCLK_REGS->GCLK_PCHCTRL[17] & 0x00000040) == 0) asm("nop");
	
	// Initialize the peripheral's context structure
	memset(&pm_ctx_uart, 0, sizeof(pm_ctx_uart));
	pm_ctx_uart.regs = UART_REGS;
	
	/*
	 * This is the classic "SWRST" (software-triggered reset).
	 * 
	 * NOTE: Like the TC peripheral, SERCOM has differing views depending
	 *       on operating mode (USART_INT for UART mode). CTRLA is shared
	 *       across all modes, so set it first after reset.
	 */
	UART_REGS->SERCOM_CTRLA = (0x1 << 0);
	while((UART_REGS->SERCOM_SYNCBUSY & (0x1 << 0)) != 0) asm("nop");
	UART_REGS->SERCOM_CTRLA = (uint32_t)(0x1 << 2);
		
	/*
	 * Select further settings compatible with the 16550 UART:
	 * 
	 * - 16-bit oversampling, arithmetic mode (for noise immunity) A
	 * - LSB first A 
	 * - No parity A
	 * - Two stop bits B
	 * - 8-bit character size B
	 * - No break detection 
	 * 
	 * - Use PAD[0] for data transmission A
	 * - Use PAD[1] for data reception A
	 * 
	 * NOTE: If a control register is not used, comment it out.
	 */
	UART_REGS->SERCOM_CTRLA |= (0x0 << 13) | (0x1 << 30) | (0x0 << 24) | (0x1 << 20);
	UART_REGS->SERCOM_CTRLB |= (0x0 << 6) | (0x0 << 0);
	//UART_REGS->SERCOM_CTRLC |= ???;
	
	/*
	 * This value is determined from f_{GCLK} and f_{baud}, the latter
	 * being the actual target baudrate (here, 9600 bps).
	 */
	UART_REGS->SERCOM_BAUD = 0xF62B;
	
	/*
	 * Configure the IDLE timeout, which should be the length of 3
	 * USART characters.
	 * 
	 * NOTE: Each character is composed of 8 bits (must include parity
	 *       and stop bits); add one bit for margin purposes. In addition,
	 *       for UART one baud period corresponds to one bit.
	 */
	pm_ctx_uart.cfg.ts_idle_timeout.nr_sec  = 0;
	pm_ctx_uart.cfg.ts_idle_timeout.nr_nsec = 781250;
	
	/*
	 * Third-to-the-last setup:
	 * 
	 * - Enable receiver and transmitter
	 * - Clear the FIFOs (even though they're disabled)
	 */
    
	UART_REGS->SERCOM_CTRLB |= (0x1 << 17) | (0x1 << 16) | (0x3 << 22);
	while ((UART_REGS->SERCOM_SYNCBUSY & (0x1 << 2)) != 0) asm("nop");
    
	/*
	 * Second-to-last: Configure the physical pins.
	 * 
	 * NOTE: Consult both the chip and board datasheets to determine the
	 *       correct port pins to use.
	 */
    PORT_SEC_REGS->GROUP[0].PORT_DIRCLR = (1 << 5);
	PORT_SEC_REGS->GROUP[0].PORT_PINCFG[5] = 0x3;
	PORT_SEC_REGS->GROUP[0].PORT_PMUX[5 >> 1] = 0x30;
    
    
    // Last: enable the peripheral, after resetting the state machine
	UART_REGS->SERCOM_CTRLA |= (0x1 << 1);
	while ((UART_REGS->SERCOM_SYNCBUSY & (0x1 << 1)) != 0) asm("nop");
	return;

#undef UART_REGS
}

// Helper abort routine for USART reception
static void pm_usart_rx_abort_helper(pm_ctx_usart_t *ctx)
{
	if (ctx->rx.desc != NULL) {
		ctx->rx.desc->compl_type = PLATFORM_USART_RX_COMPL_DATA;
		ctx->rx.desc->compl_info.data_len = ctx->rx.idx;
		ctx->rx.desc = NULL;
	}
	ctx->rx.ts_idle.nr_sec  = 0;
	ctx->rx.ts_idle.nr_nsec = 0;
	ctx->rx.idx = 0;
	return;
}

// Tick handler for the USART
static void pm_usart_tick_handler_common(
	pm_ctx_usart_t *ctx, const platform_timespec_t *tick)
{
	uint16_t status = 0x0000;
	uint8_t  data   = 0x00;
	platform_timespec_t ts_delta;
	
	// RX handling
	if ((ctx->regs->SERCOM_INTFLAG & (1 << 2)) != 0) {
		/*
		 * There are unread data
		 * 
		 * To enable readout of error conditions, STATUS must be read
		 * before reading DATA.
		 * 
		 * NOTE: Piggyback on Bit 15, as it is undefined for this
		 *       platform.
		 */
		status = ctx->regs->SERCOM_STATUS | 0x8000;
		data   = (uint8_t)(ctx->regs->SERCOM_DATA);
	}
	do {
		if (ctx->rx.desc == NULL) {
			// Nowhere to store any read data
			break;
		}

		if ((status & 0x8003) == 0x8000) {
			// No errors detected
			ctx->rx.desc->buf[ctx->rx.idx++] = data;
			ctx->rx.ts_idle = *tick;
		}
		ctx->regs->SERCOM_STATUS |= (status & 0x00F7);

		// Some housekeeping
		if (ctx->rx.idx >= ctx->rx.desc->max_len) {
			// Buffer completely filled
			pm_usart_rx_abort_helper(ctx);
			break;
		} else if (ctx->rx.idx > 0) {
			platform_tick_delta(&ts_delta, tick, &ctx->rx.ts_idle);
			if (platform_timespec_compare(&ts_delta, &ctx->cfg.ts_idle_timeout) >= 0) {
				// IDLE timeout
				pm_usart_rx_abort_helper(ctx);
				break;
			}
		}
	} while (0);
	
	// Done
	return;
}
void pm_platform_usart_tick_handler(const platform_timespec_t *tick)
{
	pm_usart_tick_handler_common(&pm_ctx_uart, tick);
}

/// Maximum number of bytes that may be sent (or received) in one transaction
#define NR_USART_CHARS_MAX (65528)

/// Maximum number of fragments for USART TX
#define NR_USART_TX_FRAG_MAX (32)

// Begin a receive transaction
static bool pm_usart_rx_busy(pm_ctx_usart_t *ctx)
{
	return (ctx->rx.desc) != NULL;
}
static bool pm_usart_rx_async(pm_ctx_usart_t *ctx, platform_usart_rx_async_desc_t *desc)
{
	// Check some items first
	if (!desc|| !desc->buf || desc->max_len == 0 || desc->max_len > NR_USART_CHARS_MAX)
		// Invalid descriptor
		return false;
	
	if ((ctx->rx.desc) != NULL)
		// Don't clobber an existing buffer
		return false;
	
	desc->compl_type = PLATFORM_USART_RX_COMPL_NONE;
	desc->compl_info.data_len = 0;
	ctx->rx.idx = 0;
	platform_tick_hrcount(&ctx->rx.ts_idle);
	ctx->rx.desc = desc;
	return true;
}

// API-visible items
bool pm_platform_usart_cdc_rx_async(platform_usart_rx_async_desc_t *desc)
{
	return pm_usart_rx_async(&pm_ctx_uart, desc);
}
bool pm_platform_usart_cdc_rx_busy(void)
{
	return pm_usart_rx_busy(&pm_ctx_uart);
}
void pm_platform_usart_cdc_rx_abort(void)
{
	pm_usart_rx_abort_helper(&pm_ctx_uart);
}
```

## File: platform/systick.c
```cpp
/**
 * @file platform/systick.c
 * @brief Platform-support routines, SysTick component
 *
 * @author Alberto de Villa <alberto.de.villa@eee.upd.edu.ph>
 * @date   28 Oct 2024
 */

/*
 * PIC32CM5164LS00048 initial configuration:
 * -- Architecture: ARMv8 Cortex-M23
 * -- GCLK_GEN0: OSC16M @ 4 MHz, no additional prescaler
 * -- Main Clock: No additional prescaling (always uses GCLK_GEN0 as input)
 * -- Mode: Secure, NONSEC disabled
 * 
 * New clock configuration:
 * -- GCLK_GEN0: 24 MHz (DFLL48M [48 MHz], with /2 prescaler)
 * -- GCLK_GEN2: 4 MHz  (OSC16M @ 4 MHz, no additional prescaler)
 * 
 * NOTE: This file does not deal directly with hardware configuration.
 */

// Common include for the XC32 compiler
#include <xc.h>
#include <stdbool.h>
#include <string.h>

#include "../platform.h"

/////////////////////////////////////////////////////////////////////////////

// Normalize a timespec
void platform_timespec_normalize(platform_timespec_t *ts)
{
	while (ts->nr_nsec >= 1000000000) {
		ts->nr_nsec -= 1000000000;
		if (ts->nr_sec < UINT32_MAX) {
			++ts->nr_sec;
		} else {
			ts->nr_nsec = (1000000000 - 1);
			break;
		}
	}
}

// Compare two timestamps
int platform_timespec_compare(const platform_timespec_t *lhs,
	const platform_timespec_t *rhs)
{
	if (lhs->nr_sec < rhs->nr_sec)
		return -1;
	else if (lhs->nr_sec > rhs->nr_sec)
		return +1;
	else if (lhs->nr_nsec < rhs->nr_nsec)
		return -1;
	else if (lhs->nr_nsec > rhs->nr_nsec)
		return +1;
	else
		return 0;
}

/////////////////////////////////////////////////////////////////////////////

// SysTick handling
static volatile platform_timespec_t ts_wall = PLATFORM_TIMESPEC_ZERO;
static volatile uint32_t ts_wall_cookie = 0;
void __attribute__((used, interrupt())) SysTick_Handler(void)
{
	platform_timespec_t t = ts_wall;
	
	t.nr_nsec += (PLATFORM_TICK_PERIOD_US * 1000);
	while (t.nr_nsec >= 1000000000) {
		t.nr_nsec -= 1000000000;
		++t.nr_sec;	// Wrap-around intentional
	}
	
	++ts_wall_cookie;	// Wrap-around intentional
	ts_wall = t;
	++ts_wall_cookie;	// Wrap-around intentional
	
	// Reset before returning.
	SysTick->VAL  = 0x00158158;	// Any value will clear
	return;
}
#define SYSTICK_RELOAD_VAL ((24/2)*PLATFORM_TICK_PERIOD_US)
void platform_systick_init(void)
{
	/*
	 * Since SysTick might be unknown at this stage, do the following, per
	 * the Arm v8-M reference manual:
	 * 
	 * - Program LOAD
	 * - Clear (VAL)
	 * - Program CTRL
	 */
	SysTick->LOAD = SYSTICK_RELOAD_VAL;
	SysTick->VAL  = 0x00158158;	// Any value will clear
	SysTick->CTRL = 0x00000007;
	return;
}
void platform_tick_count(platform_timespec_t *tick)
{
	uint32_t cookie;
	
	// A cookie is used to make sure we get coherent data.
	do {
		cookie = ts_wall_cookie;
		*tick = ts_wall;
	} while (ts_wall_cookie != cookie);
}
void platform_tick_hrcount(platform_timespec_t *tick)
{
	platform_timespec_t t;
	uint32_t s = SYSTICK_RELOAD_VAL - SysTick->VAL;
	
	platform_tick_count(&t);
	t.nr_nsec += (1000 * s)/12;
	while (t.nr_nsec >= 1000000000) {
		t.nr_nsec -= 1000000000;
		++t.nr_sec;	// Wrap-around intentional
	}
	
	*tick = t;
}

// Difference between two ticks
void platform_tick_delta(
	platform_timespec_t *diff,
	const platform_timespec_t *lhs, const platform_timespec_t *rhs
	)
{
	platform_timespec_t d = PLATFORM_TIMESPEC_ZERO;
	uint32_t c = 0;
	
	// Seconds...
	if (lhs->nr_sec < rhs->nr_sec) {
		// Wrap-around
		d.nr_sec = (UINT32_MAX - rhs->nr_sec) + lhs->nr_sec + 1;
	} else {
		// No wrap-around
		d.nr_sec = lhs->nr_sec - rhs->nr_sec;
	}
	
	// Nano-seconds...
	if (lhs->nr_sec < rhs->nr_sec) {
		// Wrap-around
		c = rhs->nr_sec - lhs->nr_sec;
		while (c >= 1000000000) {
			c -= 1000000000;
			if (d.nr_sec == 0) {
				d.nr_sec = UINT32_MAX;
			} else {
				--d.nr_sec;
			}
		}
		if (d.nr_sec == 0) {
			d.nr_sec = UINT32_MAX;
		} else {
			--d.nr_sec;
		}
	} else {
		// No wrap-around
		d.nr_nsec = lhs->nr_nsec - rhs->nr_nsec;
	}
	
	// Normalize...
	*diff = d;
	return;
}
```

## File: platform/usart.c
```cpp
/**
 * @file platform/usart.c
 * @brief Platform-support routines, USART component
 *
 * @author Alberto de Villa <alberto.de.villa@eee.upd.edu.ph>
 * @date   28 Oct 2024
 */

/*
 * PIC32CM5164LS00048 initial configuration:
 * -- Architecture: ARMv8 Cortex-M23
 * -- GCLK_GEN0: OSC16M @ 4 MHz, no additional prescaler
 * -- Main Clock: No additional prescaling (always uses GCLK_GEN0 as input)
 * -- Mode: Secure, NONSEC disabled
 * 
 * HW configuration for the corresponding Curiosity Nano+ Touch Evaluation
 * Board:
 * -- PB08: UART via debugger (TX, SERCOM3, PAD[0])
 * -- PB09: UART via debugger (RX, SERCOM3, PAD[1])
 */

// Common include for the XC32 compiler
#include <xc.h>
#include <stdbool.h>
#include <string.h>

#include "../platform.h"

// Functions "exported" by this file
void platform_usart_init(void);
void platform_usart_tick_handler(const platform_timespec_t *tick);

/////////////////////////////////////////////////////////////////////////////

/**
 * State variables for UART
 * 
 * NOTE: Since these are shared between application code and interrupt handlers
 *       (SysTick and SERCOM), these must be declared volatile.
 */
typedef struct ctx_usart_type {
	
	/// Pointer to the underlying register set
	sercom_usart_int_registers_t *regs;
	
	/// State variables for the transmitter
	struct {
		volatile platform_usart_tx_bufdesc_t *desc;
		volatile uint16_t nr_desc;
		
		// Current descriptor
		volatile const char *buf;
		volatile uint16_t    len;
	} tx;
	
	/// State variables for the receiver
	struct {
		/// Receive descriptor, held by the client
		volatile platform_usart_rx_async_desc_t * volatile desc;
		
		/// Tick since the last character was received
		volatile platform_timespec_t ts_idle;
		
		/// Index at which to place an incoming character
		volatile uint16_t idx;
				
	} rx;
	
	/// Configuration items
	struct {
		/// Idle timeout (reception only)
		platform_timespec_t ts_idle_timeout;
	} cfg;
	
} ctx_usart_t;
static ctx_usart_t ctx_uart;

// Configure USART
void platform_usart_init(void){
	/*
	 * For ease of typing, #define a macro corresponding to the SERCOM
	 * peripheral and its internally-clocked USART view.
	 * 
	 * To avoid namespace pollution, this macro is #undef'd at the end of
	 * this function.
	 */
#define UART_REGS (&(SERCOM3_REGS->USART_INT))
	
	/*
	 * Enable the APB clock for this peripheral
	 * 
	 * NOTE: The chip resets with it enabled; hence, commented-out.
	 * 
	 * WARNING: Incorrect MCLK settings can cause system lockup that can
	 *          only be rectified via a hardware reset/power-cycle.
	 */
	// MCLK_REGS->MCLK_APB???MASK |= (1 << ???);
	
	/*
	 * Enable the GCLK generator for this peripheral
	 * 
	 * NOTE: GEN2 (4 MHz) is used, as GEN0 (24 MHz) is too fast for our
	 *       use case.
	 */
	GCLK_REGS->GCLK_PCHCTRL[20] = 0x00000042;
	while ((GCLK_REGS->GCLK_PCHCTRL[20] & 0x00000040) == 0) asm("nop");
	
	// Initialize the peripheral's context structure
	memset(&ctx_uart, 0, sizeof(ctx_uart));
	ctx_uart.regs = UART_REGS;
	
	/*
	 * This is the classic "SWRST" (software-triggered reset).
	 * 
	 * NOTE: Like the TC peripheral, SERCOM has differing views depending
	 *       on operating mode (USART_INT for UART mode). CTRLA is shared
	 *       across all modes, so set it first after reset.
	 */
	UART_REGS->SERCOM_CTRLA = (0x1 << 0);
	while((UART_REGS->SERCOM_SYNCBUSY & (0x1 << 0)) != 0) asm("nop");
	UART_REGS->SERCOM_CTRLA = (uint32_t)(0x1 << 2);
		
	/*
	 * Select further settings compatible with the 16550 UART:
	 * 
	 * - 16-bit oversampling, arithmetic mode (for noise immunity) A
	 * - LSB first A 
	 * - No parity A
	 * - One stop bit B
	 * - 8-bit character size B
	 * - No break detection 
	 * 
	 * - Use PAD[0] for data transmission A
	 * - Use PAD[1] for data reception A
	 * 
	 * NOTE: If a control register is not used, comment it out.
	 */
	UART_REGS->SERCOM_CTRLA |= (0x0 << 13) | (0x1 << 30) | (0x0 << 24) | (0x0 << 16) | (0x1 << 20);
	UART_REGS->SERCOM_CTRLB |= (0x0 << 6) | (0x0 << 0);
	//UART_REGS->SERCOM_CTRLC |= ???;
	
	/*
	 * This value is determined from f_{GCLK} and f_{baud}, the latter
	 * being the actual target baudrate (here, 38400 bps).
	 */
	UART_REGS->SERCOM_BAUD = 0xF62B;
	
	/*
	 * Configure the IDLE timeout, which should be the length of 3
	 * USART characters.
	 * 
	 * NOTE: Each character is composed of 8 bits (must include parity
	 *       and stop bits); add one bit for margin purposes. In addition,
	 *       for UART one baud period corresponds to one bit.
	 */
	ctx_uart.cfg.ts_idle_timeout.nr_sec  = 0;
	ctx_uart.cfg.ts_idle_timeout.nr_nsec = 781250;
	
	/*
	 * Third-to-the-last setup:
	 * 
	 * - Enable receiver and transmitter
	 * - Clear the FIFOs (even though they're disabled)
	 */
	UART_REGS->SERCOM_CTRLB |= (0x1 << 17) | (0x1 << 16) | (0x3 << 22);
	while ((UART_REGS->SERCOM_SYNCBUSY & (0x1 << 2)) != 0) asm("nop");
	
	/*
	 * Second-to-last: Configure the physical pins.
	 * 
	 * NOTE: Consult both the chip and board datasheets to determine the
	 *       correct port pins to use.
	 */
	// <insert code here>
    PORT_SEC_REGS->GROUP[1].PORT_DIRCLR = (1 << 8) | (1 << 9);
    
	PORT_SEC_REGS->GROUP[1].PORT_PINCFG[8] = 0x03;
    
	PORT_SEC_REGS->GROUP[1].PORT_PMUX[4] = 0x3;
    
    // Last: enable the peripheral, after resetting the state machine
	UART_REGS->SERCOM_CTRLA |= (0x1 << 1);
	while ((UART_REGS->SERCOM_SYNCBUSY & (0x1 << 1)) != 0) asm("nop");
	return;

#undef UART_REGS
}

// Helper abort routine for USART reception
static void usart_rx_abort_helper(ctx_usart_t *ctx)
{
	if (ctx->rx.desc != NULL) {
		ctx->rx.desc->compl_type = PLATFORM_USART_RX_COMPL_DATA;
		ctx->rx.desc->compl_info.data_len = ctx->rx.idx;
		ctx->rx.desc = NULL;
	}
	ctx->rx.ts_idle.nr_sec  = 0;
	ctx->rx.ts_idle.nr_nsec = 0;
	ctx->rx.idx = 0;
	return;
}

// Tick handler for the USART
static void usart_tick_handler_common(
	ctx_usart_t *ctx, const platform_timespec_t *tick)
{
	uint16_t status = 0x0000;
	uint8_t  data   = 0x00;
	platform_timespec_t ts_delta;
	
	// TX handling
	if ((ctx->regs->SERCOM_INTFLAG & (1 << 0)) != 0) {
		if (ctx->tx.len > 0) {
			/*
			 * There is still something to transmit in the working
			 * copy of the current descriptor.
			 */
			ctx->regs->SERCOM_DATA = *(ctx->tx.buf++);
			--ctx->tx.len;
		}
		if (ctx->tx.len == 0) {
			// Load a new descriptor
			ctx->tx.buf = NULL;
			if (ctx->tx.nr_desc > 0) {
				/*
				 * There's at least one descriptor left to
				 * transmit
				 * 
				 * If either ->buf or ->len of the candidate
				 * descriptor refer to an empty buffer, the
				 * next invocation of this routine will cause
				 * the next descriptor to be evaluated.
				 */
				ctx->tx.buf = ctx->tx.desc->buf;
				ctx->tx.len = ctx->tx.desc->len;
				
				++ctx->tx.desc;
				--ctx->tx.nr_desc;
					
				if (ctx->tx.buf == NULL || ctx->tx.len == 0) {
					ctx->tx.buf = NULL;
					ctx->tx.len = 0;
				}
			} else {
				/*
				 * No more descriptors available
				 * 
				 * Clean up the corresponding context data so
				 * that we don't trip over them on the next
				 * invocation.
				 */
				ctx->regs->SERCOM_INTENCLR = 0x01;
				ctx->tx.desc = NULL;
				ctx->tx.buf = NULL;
			}
		}
	}
    
	// Done
	return;
}
void platform_usart_tick_handler(const platform_timespec_t *tick)
{
	usart_tick_handler_common(&ctx_uart, tick);
}

/// Maximum number of bytes that may be sent (or received) in one transaction
#define NR_USART_CHARS_MAX (65528)

/// Maximum number of fragments for USART TX
#define NR_USART_TX_FRAG_MAX (32)

// Enqueue a buffer for transmission
static bool usart_tx_busy(ctx_usart_t *ctx)
{
	return (ctx->tx.len > 0) || (ctx->tx.nr_desc > 0) ||
		((ctx->regs->SERCOM_INTFLAG & (1 << 0)) == 0);
}
static bool usart_tx_async(ctx_usart_t *ctx,
	const platform_usart_tx_bufdesc_t *desc,
	unsigned int nr_desc)
{
	uint16_t avail = NR_USART_CHARS_MAX;
	unsigned int x, y;
	
	if (!desc || nr_desc == 0)
		return true;
	else if (nr_desc > NR_USART_TX_FRAG_MAX)
		// Too many descriptors
		return false;
	
	// Don't clobber an existing buffer
	if (usart_tx_busy(ctx))
		return false;
	
	for (x = 0, y = 0; x < nr_desc; ++x) {
		if (desc[x].len > avail) {
			// IF the message is too long, don't enqueue.
			return false;
		}
		
		avail -= desc[x].len;
		++y;
	}
	
	// The tick will trigger the transfer
	ctx->tx.desc = desc;
	ctx->tx.nr_desc = nr_desc;
	return true;
}
static void usart_tx_abort(ctx_usart_t *ctx)
{
	ctx->tx.nr_desc = 0;
	ctx->tx.desc = NULL;
	ctx->tx.len = 0;
	ctx->tx.buf = NULL;
	return;
}

// API-visible items
bool platform_usart_cdc_tx_async(
	const platform_usart_tx_bufdesc_t *desc,
	unsigned int nr_desc)
{
	return usart_tx_async(&ctx_uart, desc, nr_desc);
}
bool platform_usart_cdc_tx_busy(void)
{
	return usart_tx_busy(&ctx_uart);
}
void platform_usart_cdc_tx_abort(void)
{
	usart_tx_abort(&ctx_uart);
	return;
}

// Begin a receive transaction
static bool usart_rx_busy(ctx_usart_t *ctx)
{
	return (ctx->rx.desc) != NULL;
}
static bool usart_rx_async(ctx_usart_t *ctx, platform_usart_rx_async_desc_t *desc)
{
	// Check some items first
	if (!desc|| !desc->buf || desc->max_len == 0 || desc->max_len > NR_USART_CHARS_MAX)
		// Invalid descriptor
		return false;
	
	if ((ctx->rx.desc) != NULL)
		// Don't clobber an existing buffer
		return false;
	
	desc->compl_type = PLATFORM_USART_RX_COMPL_NONE;
	desc->compl_info.data_len = 0;
	ctx->rx.idx = 0;
	platform_tick_hrcount(&ctx->rx.ts_idle);
	ctx->rx.desc = desc;
	return true;
}

// API-visible items
bool platform_usart_cdc_rx_async(platform_usart_rx_async_desc_t *desc)
{
	return usart_rx_async(&ctx_uart, desc);
}
bool platform_usart_cdc_rx_busy(void)
{
	return usart_rx_busy(&ctx_uart);
}
void platform_usart_cdc_rx_abort(void)
{
	usart_rx_abort_helper(&ctx_uart);
}
```

## File: convert.py
```python
import binascii
import time

TIME_DELAY = 1.01

filename = "putty.log"
def byte_to_string(line: bytes) -> str:
    return line.replace(starting_bits, b'\n' + starting_bits).decode("utf-8")

# ! The commented portions assume data is represented in integers.
# def string_to_hex(string: str, data_num: int) -> str:
#     first_part = string[0:2]
#     second_part = string[2:4]
#     #print(f"Data {data_num} Hex: {string} | {first_part} | {second_part}")
#     return f"{int(first_part, 16)}.{int(second_part, 16)}"


while True:
    with open(filename, 'rb') as f:
        content = f.read()
    hex_content = binascii.hexlify(content)
    starting_bits = b'424d'
    
    last_line_b = hex_content.split(starting_bits)[::-1][0]
    last_line_str = byte_to_string(last_line_b)
    # !The last line does not include 0x424d
    # We choose the particles under atmospheric conditions per the datasheet.
    data_1 = last_line_str[16:20]
    data_2 = last_line_str[20:24]
    data_3 = last_line_str[24:28]
    with open("convert.log", "a") as f:
        # print(f"Hex: {last_line_str}")
        # print(f"Hex: {last_line_str}", file=f)
        try:
            # print(f"PM1.0: {string_to_hex(data_1, 1)} | PM2.5: {string_to_hex(data_2, 2)} | PM10: {string_to_hex(data_3, 3)} || Unit: ug/m3")
            # print(f"PM1.0: {string_to_hex(data_1, 1)} | PM2.5: {string_to_hex(data_2, 2)} | PM10: {string_to_hex(data_3, 3)} || Unit: ug/m3", file=f)
            # print("-"*10, "No decimal", "-"*10)
            print(f"PM1.0: {int(data_1, 16)} | PM 2.5: {int(data_2, 16)} | PM 10: {int(data_3, 16)} || Unit: ug/m3")
            print(f"PM1.0: {int(data_1, 16)} | PM 2.5: {int(data_2, 16)} | PM 10: {int(data_3, 16)} || Unit: ug/m3", file=f)
        except:
            print("Error! Bits are short... looping again...")
            print("Error! Bits are short... looping again...", file=f)
            time.sleep(TIME_DELAY)
            continue
    time.sleep(TIME_DELAY)
```
