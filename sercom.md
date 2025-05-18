# 33. Serial Communication Interface (SERCOM)

## 33.1 Overview
There are up to six instances of the serial communication interface (SERCOM) peripheral.
A SERCOM can be configured to support a number of modes: I2C, SPI, and USART. When an instance of SERCOM is configured and enabled, all of the resources of that SERCOM instance will be dedicated to the selected mode.
The SERCOM serial engine consists of a transmitter and receiver, baud-rate generator and address matching functionality. It can use the internal generic clock or an external clock. Using an external clock allows the SERCOM to be operated in all Sleep modes.

## 33.2 Features
The following are key features of the SERCOM module:
*   Interface for configuring into one of the following:
    *   Inter-Integrated Circuit (I2C) Two-wire Serial Interface
    *   System Management Bus (SMBus™) compatible
    *   Serial Peripheral Interface (SPI)
    *   Universal Synchronous/Asynchronous Receiver/Transmitter (USART)
*   Baud-rate generator
*   Address match/mask logic
*   Operational in all Sleep modes with an external clock source
*   Can be used with DMA
*   Receive buffer: 8-bytes FIFO
*   Transmit buffer: 8-bytes FIFO
*   32-bit extension for better system bus utilization
*   Secure pin multiplexing to isolate on dedicated I/O pins a secured communication with external devices from the non-secure application (PIC32CM LS00 only)

The following table lists the supported features for each SERCOM instance:

Table 33-1. SERCOM Features Summary (PIC32CM LE00 and PIC32CM LS00)
| Protocol                                  | SERCOM Instance |       |         |         |         |         |
|-------------------------------------------|-----------------|-------|---------|---------|---------|---------|
|                                           | SERCOM0         | SERCOM1 | SERCOM2 | SERCOM3 | SERCOM4 | SERCOM5 |
| SPI                                       | Yes             | Yes     | Yes     | Yes     | Yes     | Yes     |
| USART                                     | Yes             | Yes     | Yes     | Yes     | Yes     | Yes     |
| I2C                                       | Yes             | Yes     | Yes     | Yes(1)  | Yes     | Yes(1)  |
| Secure Pin Multiplexing (PIC32CM LS00 only) | No              | Yes     | No      | No      | No      | No      |

**Notes:**
1.  I2C is not supported on SERCOM3 and SERCOM5 for 48-pin packages. Refer to the Pinout for more information.
2.  SERCOM3, SERCOM4, and SERCOM5 are not present on the PIC32CM1216 devices.

Table 33-2. SERCOM Features Summary (PIC32CM LS60)
| Protocol                | SERCOM Instance                                  |         |         |         |         |
|-------------------------|--------------------------------------------------|---------|---------|---------|---------|
|                         | SERCOM0                                          | SERCOM1 | SERCOM2 | SERCOM3 | SERCOM4 | SERCOM5 |
| SPI                     | Yes                                              | Yes     | Yes     | Yes     | Yes     | Yes     |
| USART                   | Yes                                              | Yes     | Yes     | Yes     | Yes     | Yes     |
| I2C                     | Yes                                              | Reserved for ATECC608B (I2C) | Yes     | Yes(1)  | Yes     | Yes(1)  |
| Secure Pin Multiplexing | No                                               | Yes     | No      | No      | No      | No      |

**Note:**
1.  I2C is not supported on SERCOM3 and SERCOM5 for 48-pin packages. Refer to the Pinout for more information.

## 33.3 Block Diagram
Figure 33-1. SERCOM Block Diagram
*(Block diagram content ignored as per instruction)*

## 33.4 Signal Description
Refer to the respective SERCOM mode chapters for details:
*   SERCOM USART
*   SERCOM SPI
*   SERCOM I2C

**CAUTION**
I/Os for SERCOM peripherals are grouped into I/O sets, listed in their 'IOSET' column in the pinout tables. For these peripherals, it is mandatory to use I/Os that belong to the same I/O set. The timings are not guaranteed when I/Os from different I/O sets are mixed. Refer to the Pinout and Packaging chapter to get IOSET definitions.

## 33.5 Peripheral Dependencies

Table 33-3. SERCOM Configuration Summary
| Peripheral name | Base address | NVIC IRQ Index | MCLK AHB/APB Clocks   | GCLK Peripheral Channel Index (GCLK.PCHCTRL) | PAC Peripheral Identifier Index (PAC.WRCTRL) | DMA Trigger Source Index (DMAC.CHCTRLB) | Power Domain (PM.STDBYCFG) |
|-----------------|--------------|----------------|-----------------------|------------------------------------------------|----------------------------------------------|-----------------------------------------|----------------------------|
| SERCOM0         | 0x42000400   | 31: bit 0      | CLK_SERCOM0_APB       | 17: GCLK_SERCOM0_CORE <br> GCLK_SERCOM0_SLOW   | 1                                            | 4: RX <br> 5: TX                        | PDSW                       |
|                 |              | 32: bit 1      |                       |                                                |                                              |                                         |                            |
|                 |              | 33: bit 2      |                       |                                                |                                              |                                         |                            |
|                 |              | 34: bit 3-7    |                       |                                                |                                              |                                         |                            |
| SERCOM1         | 0x42000800   | 35: bit 0      | CLK_SERCOM1_APB       | 18: GCLK_SERCOM1_CORE <br> GCLK_SERCOM1_SLOW   | 2                                            | 6: RX <br> 7: TX                        | PDSW                       |
|                 |              | 36: bit 1      |                       |                                                |                                              |                                         |                            |
|                 |              | 37: bit 2      |                       |                                                |                                              |                                         |                            |
|                 |              | 38: bit 3-7    |                       |                                                |                                              |                                         |                            |
| SERCOM2         | 0x42000C00   | 39: bit 0      | CLK_SERCOM2_APB       | 19: GCLK_SERCOM2_CORE <br> GCLK_SERCOM2_SLOW   | 3                                            | 8: RX <br> 9: TX                        | PDSW                       |
|                 |              | 40: bit 1      |                       |                                                |                                              |                                         |                            |
|                 |              | 41: bit 2      |                       |                                                |                                              |                                         |                            |
|                 |              | 42: bit 3-7    |                       |                                                |                                              |                                         |                            |
| SERCOM3         | 0x42001000   | 43: bit 0      | CLK_SERCOM3_APB       | 20: GCLK_SERCOM3_CORE <br> GCLK_SERCOM3_SLOW   | 4                                            | 10: RX <br> 11: TX                       | PDSW                       |
|                 |              | 44: bit 1      |                       |                                                |                                              |                                         |                            |
|                 |              | 45: bit 2      |                       |                                                |                                              |                                         |                            |
|                 |              | 46: bit 3-7    |                       |                                                |                                              |                                         |                            |
| SERCOM4         | 0x42001400   | 47: bit 0      | CLK_SERCOM4_APB       | 21: GCLK_SERCOM4_CORE <br> GCLK_SERCOM4_SLOW   | 5                                            | 12: RX <br> 13: TX                       | PDSW                       |
|                 |              | 48: bit 1      |                       |                                                |                                              |                                         |                            |
|                 |              | 49: bit 2      |                       |                                                |                                              |                                         |                            |
|                 |              | 50: bit 3-7    |                       |                                                |                                              |                                         |                            |
| SERCOM5         | 0x42001800   | 51: bit 0      | CLK_SERCOM5_APB       | 22: GCLK_SERCOM5_CORE <br> GCLK_SERCOM5_SLOW   | 6                                            | 14: RX <br> 15: TX                       | PDSW                       |
|                 |              | 52: bit 1      |                       |                                                |                                              |                                         |                            |
|                 |              | 53: bit 2      |                       |                                                |                                              |                                         |                            |
|                 |              | 54: bit 3-7    |                       |                                                |                                              |                                         |                            |

**Note:** SERCOM3, SERCOM4, and SERCOM5 are not present on the PIC32CM1216 devices.

## 33.6 Functional Description

### 33.6.1 Principle of Operation
The SERCOM has four internal pads, PAD[3:0], and the signals from I2C, SPI, and USART are routed through these SERCOM pads through a multiplexer. The configuration of the multiplexer is available from the different SERCOM modes. For additional information, refer to these chapters:
*   SERCOM USART
*   SERCOM SPI
*   SERCOM I2C

The SERCOM uses up to two generic clocks: `GCLK_SERCOMx_CORE` and `GCLK_SERCOMx_SLOW`. The core clock (`GCLK_SERCOMx_CORE`) is required to clock the SERCOM while working as a Host. The slow clock (`GCLK_SERCOMx_SLOW`) is only required for certain functions. Refer to the specific mode chapters for additional information.

The basic structure of the SERCOM serial engine is shown in the following figure. Labels in capital letters are synchronous to the system clock and accessible by the CPU; labels in lowercase letters can be configured to run on the `GCLK_SERCOMx_CORE` clock or an external clock.

Figure 33-2. SERCOM Serial Engine
*(Block diagram content ignored as per instruction)*

The transmitter consists of a minimum of 8-bytes write buffer and a Shift register.
The receiver consists of a minimum of 8-bytes receive buffer and a Shift register.
The Baud Rate Generator (BRG) is capable of running on the `GCLK_SERCOMx_CORE` clock or an external clock.
Address matching logic is included for SPI and I2C operation.

### 33.6.2 Basic Operation

#### 33.6.2.1 Initialization
The SERCOM must be configured to the desired mode by writing the Operating Mode bits in the Control A register (CTRLA.MODE) as shown in the table below.

Table 33-4. SERCOM Modes
| CTRLA.MODE | Description                |
|------------|----------------------------|
| 0x0        | USART with external clock  |
| 0x1        | USART with internal clock  |
| 0x2        | SPI in client operation    |
| 0x3        | SPI in host operation      |
| 0x4        | I2C client operation       |
| 0x5        | I2C host operation         |
| 0x6-0x7    | Reserved                   |

For further initialization information, see the respective SERCOM mode chapters:
*   SERCOM USART
*   SERCOM SPI
*   SERCOM I2C

#### 33.6.2.2 Enabling, Disabling, and Resetting
This peripheral is enabled by writing '1' to the Enable bit in the Control A register (CTRLA.ENABLE), and disabled by writing '0' to it.
Writing '1' to the Software Reset bit in the Control A register (CTRLA.SWRST) will reset all registers of this peripheral to their initial states, except the DBGCTRL register, and the peripheral is disabled.
Refer to the CTRLA register description for details.

#### 33.6.2.3 Clock Generation - Baud-Rate Generator
The baud-rate generator, as shown in the following figure, generates internal clocks for asynchronous and synchronous communication. The output frequency (f<sub>BAUD</sub>) is determined by the Baud register (BAUD) setting and the baud reference frequency (f<sub>ref</sub>). The baud reference clock is the serial engine clock, and it can be internal or external.
For asynchronous communication, the /16 (divide-by-16) output is used when transmitting, whereas the /1 (divide-by-1) output is used while receiving.
For synchronous communication, the /2 (divide-by-2) output is used.
This functionality is automatically configured, depending on the selected operating mode.

Figure 33-3. Baud Rate Generator
*(Block diagram content ignored as per instruction)*

The following table contains equations for the baud rate (in bits per second) and the BAUD register value for each operating mode.
For asynchronous operation, there are two modes:
*   **Arithmetic mode:** the BAUD register value is 16 bits (0 to 65,535).
*   **Fractional mode:** the BAUD register value is 13 bits, while the fractional adjustment is 3 bits. In this mode the BAUD setting must be greater than or equal to 1.
For synchronous operation, the BAUD register value is 8 bits (0 to 255).

Table 33-5. Baud Rate Equations
| Operating Mode          | Condition                     | Baud Rate (Bits Per Second)                        | BAUD Register Value Calculation                     |
|-------------------------|-------------------------------|----------------------------------------------------|---------------------------------------------------|
| Asynchronous Arithmetic | f<sub>BAUD</sub> ≤ f<sub>ref</sub> / S        | f<sub>BAUD</sub> = f<sub>ref</sub> / (S * (1 - BAUD / 65536))  | BAUD = 65536 * (1 - f<sub>ref</sub> / (S * f<sub>BAUD</sub>)) |
| Asynchronous Fractional | f<sub>BAUD</sub> ≤ f<sub>ref</sub> / S        | f<sub>BAUD</sub> = f<sub>ref</sub> / (S * (BAUD + FP / 8))       | BAUD = f<sub>ref</sub> / (S * f<sub>BAUD</sub>) - FP / 8          |
| Synchronous             | f<sub>BAUD</sub> ≤ f<sub>ref</sub> / 2        | f<sub>BAUD</sub> = f<sub>ref</sub> / (2 * (BAUD + 1))          | BAUD = f<sub>ref</sub> / (2 * f<sub>BAUD</sub>) - 1             |

S - Number of samples per bit, which can be 16, 8, or 3.
The Asynchronous Fractional option is used for auto-baud detection.
The baud rate error is represented by the following formula:
Error = 1 - (ExpectedBaudRate / ActualBaudRate)

##### 33.6.2.3.1 Asynchronous Arithmetic Mode BAUD Value Selection
The formula given for f<sub>BAUD</sub> calculates the average frequency over 65536 f<sub>ref</sub> cycles. Although the BAUD register can be set to any value between 0 and 65536, the actual average frequency of f<sub>BAUD</sub> over a single frame is more granular. The BAUD register values that will affect the average frequency over a single frame lead to an integer increase in the cycles per frame (CPF).
CPF = f<sub>ref</sub> / f<sub>BAUD</sub> = (D + S)
where
*   D represent the data bits per frame
*   S represent the sum of start and first stop bits, if present.

Table 33-6 shows the BAUD register value versus baud frequency f<sub>BAUD</sub> at a serial engine frequency of 48MHz. This assumes a D value of 8 bits and an S value of 2 bits (10 bits, including start and stop bits).

Table 33-6. BAUD Register Value vs. Baud Frequency
| BAUD Register Value | Serial Engine CPF | f<sub>BAUD</sub> at 48MHz Serial Engine Frequency (f<sub>ref</sub>) |
|---------------------|-------------------|-----------------------------------------------------------|
| 0 - 406             | 160               | 3MHz                                                      |
| 407 - 808           | 161               | 2.981MHz                                                  |
| 809 - 1205          | 162               | 2.963MHz                                                  |
| ...                 | ...               | ...                                                       |
| 65206               | 31775             | 15.11kHz                                                  |
| 65207               | 31871             | 15.06kHz                                                  |
| 65208               | 31969             | 15.01kHz                                                  |

### 33.6.3 Additional Features

#### 33.6.3.1 Address Match and Mask
The SERCOM address match and mask feature is capable of matching either one address, two unique addresses, or a range of addresses with a mask, based on the mode selected. The match uses seven or eight bits, depending on the mode.

##### 33.6.3.1.1 Address With Mask
An Address written to the Address bits in the Address register (ADDR.ADDR), and a mask written to the Address Mask bits in the Address register (ADDR.ADDRMASK) will yield an address match. All bits that are masked are not included in the match. Note that writing the ADDR.ADDRMASK to 'all zeros' will match a single unique address, while writing ADDR.ADDRMASK to 'all ones' will result in all addresses being accepted.

Figure 33-4. Address With Mask
*(Block diagram content ignored as per instruction)*

##### 33.6.3.1.2 Two Unique Addresses
The two addresses written to ADDR and ADDRMASK will cause a match.

Figure 33-5. Two Unique Addresses
*(Block diagram content ignored as per instruction)*

##### 33.6.3.1.3 Address Range
The range of addresses between and including ADDR.ADDR and ADDR.ADDRMASK will cause a match. ADDR.ADDR and ADDR.ADDRMASK can be set to any two addresses, with ADDR.ADDR acting as the upper limit and ADDR.ADDRMASK acting as the lower limit.

Figure 33-6. Address Range
*(Block diagram content ignored as per instruction)*

#### 33.6.3.2 Secure Pin Multiplexing (PIC32CM LS00 only)
The Secure Pin Multiplexing feature can be used on dedicated SERCOM1 I/O pins to isolate a secure communication with external devices from the non-secure application.
Secure Pin Multiplexing is automatically enabled as soon as SERCOM1 is configured as a secure peripheral (PAC Security attribution).
The following table lists the SERCOM1 pins that support the Secure Pin Multiplexing feature:

Table 33-7. Secure Pin Multiplexing on SERCOM1 Pins (PIC32CM LS00 only)
| Pin Name | Secure Pin Multiplexing Pad Name |
|----------|----------------------------------|
| PA16     | SERCOM1/PAD[0]                   |
| PA17     | SERCOM1/PAD[1]                   |
| PA18     | SERCOM1/PAD[2]                   |
| PA19     | SERCOM1/PAD[3]                   |

### 33.6.4 DMA Operation
The available DMA interrupts depend on the operation mode of the SERCOM peripheral. Refer to the Functional Description sections of the respective SERCOM mode:
*   SERCOM USART
*   SERCOM SPI
*   SERCOM I2C

### 33.6.5 Interrupts
Interrupt sources are mode-specific. See the respective SERCOM mode chapters for details.
Each interrupt source has its own interrupt flag.
The interrupt flag in the Interrupt Flag Status and Clear register (INTFLAG) will be set when the interrupt condition is met.
Each interrupt can be individually enabled by writing '1' to the corresponding bit in the Interrupt Enable Set register (INTENSET), and disabled by writing '1' to the corresponding bit in the Interrupt Enable Clear register (INTENCLR).
An interrupt request is generated when the interrupt flag is set and the corresponding interrupt is enabled. The interrupt request remains active until either the interrupt flag is cleared, the interrupt is disabled, or the SERCOM is reset. For details on clearing interrupt flags, refer to the INTFLAG register description.
The value of INTFLAG indicates which interrupt condition occurred. The user must read the INTFLAG register to determine which interrupt condition is present.
**Note:** Interrupts must be globally enabled for interrupt requests. See the 11.2. Nested Vector Interrupt Controller for more information.

### 33.6.6 Sleep Mode Operation
The peripheral can operate in any sleep mode where the selected serial clock is running. This clock can be external or generated by the internal baud-rate generator.
The SERCOM interrupts can be used to wake up the device from sleep modes. Refer to the different SERCOM mode chapters for details:
*   SERCOM USART
*   SERCOM SPI
*   SERCOM I2C

### 33.6.7 Debug Operation
When the CPU is halted in Debug mode, this peripheral will continue normal operation. If the peripheral is configured to require periodical service by the CPU through interrupts or similar, improper operation or data loss may result during debugging. This peripheral can be forced to halt operation during debugging - refer to the Debug Control (DBGCTRL) register for details.

### 33.6.8 Synchronization
Due to asynchronicity between the main clock domain and the peripheral clock domains, some registers need to be synchronized when written or read.
Required write synchronization is denoted by the "Write-Synchronized" property in the register description.
Required read synchronization is denoted by the "Read-Synchronized" property in the register description.