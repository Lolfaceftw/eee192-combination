# 34. Universal Synchronous and Asynchronous Receiver-Transmitter (SERCOM USART)

## 34.1 Overview
The Universal Synchronous and Asynchronous Receiver and Transmitter (USART) is one of the available modes in the Serial Communication Interface (SERCOM).

The USART uses the SERCOM transmitter and receiver, see Section 34.3. Block Diagram. Labels in uppercase letters are synchronous to `CLK_SERCOMx_APB` and accessible for CPU. Labels in lowercase letters can be programmed to run on the internal generic clock or an external clock.

The transmitter consists of a 8-bytes write FIFO buffer, a Shift register, and control logic for different frame formats. The write buffer support data transmission without any delay between frames.

The receiver consists of a 8-bytes receive FIFO buffer and a Shift register. Status information of the received data is available for error checking. Data and clock recovery units ensure robust synchronization and noise filtering during asynchronous data reception.

## 34.2 USART Features
*   Full-duplex operation
*   Asynchronous (with clock reconstruction) or synchronous operation
*   Internal or external clock source for asynchronous and synchronous operation
*   Baud-rate generator
*   Supports serial frames with 5, 6, 7, 8 or 9 data bits and 1 or 2 stop bits
*   Odd or even parity generation and parity check
*   Selectable LSB- or MSB-first data transfer
*   Buffer overflow and frame error detection
*   Noise filtering, including false start-bit detection and digital low-pass filter
*   Collision detection
*   Sleep modes operation supported
*   RTS and CTS flow control
*   IrDA modulation and demodulation up to 115.2kbps
*   LIN Host support
*   LIN Client support
    *   Auto-baud and break character detection
*   ISO7816 T=0 or T=1 protocols for Smart Card interfacing
*   RS485 Support
*   Start-of-frame detection
*   Receive buffer: 8-bytes FIFO
*   Transmit buffer: 8-bytes FIFO
*   DMA operations supported
*   32-bit extension for better system bus utilization

## 34.3 Block Diagram
Figure 34-1. USART Block Diagram
*(Block diagram content ignored as per instruction)*

## 34.4 Signal Description

Table 34-1. SERCOM USART Signals
| Signal Name | Type        | Description           |
|-------------|-------------|-----------------------|
| PAD[3:0]    | Digital I/O | General SERCOM pins   |

**CAUTION**
I/Os for SERCOM peripherals are grouped into IO sets, listed in their 'IOSET' column in the pinout tables. For these peripherals, it is mandatory to use I/Os that belong to the same IO set. The timings are not guaranteed when IOs from different IO sets are mixed. Refer to the Pinout and Packaging chapter to get IOSET definitions.

## 34.5 Peripheral Dependencies

Table 34-2. SERCOM Configuration Summary
| Peripheral name | Base address | NVIC IRQ Index | MCLK AHB/APB Clocks | GCLK Peripheral Channel Index (GCLK.PCHCTRL) | PAC Peripheral Identifier Index (PAC.WRCTRL) | DMA Trigger Source Index (DMAC.CHCTRLB) | Power Domain (PM.STDBYCFG) |
|-----------------|--------------|----------------|---------------------|----------------------------------------------|----------------------------------------------|-----------------------------------------|----------------------------|
| SERCOM0         | 0x42000400   | 31: bit 0      | CLK_SERCOM0_APB     | 17: GCLK_SERCOM0_CORE                        | 65                                           | 4: RX <br> 5: TX                        | PDSW                       |
|                 |              | 32: bit 1      |                     |                                              |                                              |                                         |                            |
|                 |              | 33: bit 2      |                     |                                              |                                              |                                         |                            |
|                 |              | 34: bit 3-7    |                     |                                              |                                              |                                         |                            |
| SERCOM1         | 0x42000800   | 35: bit 0      | CLK_SERCOM1_APB     | 18: GCLK_SERCOM1_CORE                        | 66                                           | 6: RX <br> 7: TX                        | PDSW                       |
|                 |              | 36: bit 1      |                     |                                              |                                              |                                         |                            |
|                 |              | 37: bit 2      |                     |                                              |                                              |                                         |                            |
|                 |              | 38: bit 3-7    |                     |                                              |                                              |                                         |                            |
| SERCOM2         | 0x42000C00   | 39: bit 0      | CLK_SERCOM2_APB     | 19: GCLK_SERCOM2_CORE                        | 67                                           | 8: RX <br> 9: TX                        | PDSW                       |
|                 |              | 40: bit 1      |                     |                                              |                                              |                                         |                            |
|                 |              | 41: bit 2      |                     |                                              |                                              |                                         |                            |
|                 |              | 42: bit 3-7    |                     |                                              |                                              |                                         |                            |
| SERCOM3         | 0x42001000   | 43: bit 0      | CLK_SERCOM3_APB     | 20: GCLK_SERCOM3_CORE                        | 68                                           | 10: RX <br> 11: TX                       | PDSW                       |
|                 |              | 44: bit 1      |                     |                                              |                                              |                                         |                            |
|                 |              | 45: bit 2      |                     |                                              |                                              |                                         |                            |
|                 |              | 46: bit 3-7    |                     |                                              |                                              |                                         |                            |
| SERCOM4         | 0x42001400   | 47: bit 0      | CLK_SERCOM4_APB     | 21: GCLK_SERCOM4_CORE                        | 69                                           | 12: RX <br> 13: TX                       | PDSW                       |
|                 |              | 48: bit 1      |                     |                                              |                                              |                                         |                            |
|                 |              | 49: bit 2      |                     |                                              |                                              |                                         |                            |
|                 |              | 50: bit 3-7    |                     |                                              |                                              |                                         |                            |
| SERCOM5         | 0x42001800   | 51: bit 0      | CLK_SERCOM5_APB     | 22: GCLK_SERCOM5_CORE                        | 70                                           | 14: RX <br> 15: TX                       | PDSW                       |
|                 |              | 52: bit 1      |                     |                                              |                                              |                                         |                            |
|                 |              | 53: bit 2      |                     |                                              |                                              |                                         |                            |
|                 |              | 54: bit 3-7    |                     |                                              |                                              |                                         |                            |

## 34.6 Functional Description

### 34.6.1 Principle of Operation
The USART uses the following lines for data transfer:
*   RxD for receiving
*   TxD for transmitting
*   XCK for the transmission clock in synchronous operation

When the SERCOM is used in USART mode, the SERCOM controls the direction and value of the I/O pins according to the table below.

Table 34-3. USART Pin Configuration
| Pin | Pin Configuration |
|-----|-------------------|
| TxD | Output            |
| RxD | Input             |
| XCK | Output or input   |

PORT Control bit `PINCFGn.DRVSTR` is still effective for the SERCOM output pins.
PORT Control bit `PINCFGn.PULLEN` is still effective on the SERCOM input pins, but is limited to the enabling/disabling of a pull down only (it is not possible to enable/disable a pull up).
If the receiver or transmitter is disabled, these pins can be used for other purposes.

The combined configuration of PORT and the Transmit Data Pinout and Receive Data Pinout bit fields in the Control A register (`CTRLA.TXPO` and `CTRLA.RXPO`, respectively) will define the physical position of the USART signals in the Pin Configuration Summary.

USART data transfer is frame based. A serial frame consists of:
*   1 start bit
*   From 5 to 9 data bits (MSB or LSB first)
*   No, even or odd parity bit
*   1 or 2 stop bits

A frame starts with the start bit followed by one character of data bits. If enabled, the parity bit is inserted after the data bits and before the first stop bit. After the stop bit(s) of a frame, either the next frame can follow immediately, or the communication line can return to the idle (high) state.
Figure 34-2. Frame Formats
*(Figure content ignored)*
St: Start bit. Signal is always low.
n, [n]: Data bits. 0 to [5..9]
[P]: Parity bit. Either odd or even.
Sp, [Sp]: Stop bit. Signal is always high.
IDLE: No frame is transferred on the communication line. Signal is always high in this state.

### 34.6.2 Initialization

#### 34.6.2.1 Initialization
The following registers are enable-protected, meaning they can only be written when the USART is disabled (`CTRLA.ENABLE=0`):
*   Control A register (CTRLA), except the Enable (ENABLE) and Software Reset (SWRST) bits.
*   Control B register (CTRLB), except the Receiver Enable (RXEN) and Transmitter Enable (TXEN) bits.
*   Baud register (BAUD)
*   Control C register (CTRLC)
*   Receive Pulse Length register (RXPL)

When the USART is enabled or is being enabled (`CTRLA.ENABLE=1`), any writing attempt to these registers will be discarded. If the peripheral is being disabled, writing to these registers will be executed after disabling is completed. Enable-protection is denoted by the "Enable-Protection" property in the register description.

Before the USART is enabled, it must be configured by these steps:
1.  Select either external (0x0) or internal clock (0x1) by writing the Operating Mode value in the `CTRLA` register (`CTRLA.MODE`).
2.  Select either asynchronous (0) or synchronous (1) communication mode by writing the Communication Mode bit in the `CTRLA` register (`CTRLA.CMODE`).
3.  Select pin for receive data by writing the Receive Data Pinout value in the `CTRLA` register (`CTRLA.RXPO`).
4.  Select pads for the transmitter and external clock by writing the Transmit Data Pinout bit in the `CTRLA` register (`CTRLA.TXPO`).
5.  Configure the Character Size field in the `CTRLB` register (`CTRLB.CHSIZE`) for character size.
6.  Set the Data Order bit in the `CTRLA` register (`CTRLA.DORD`) to determine MSB- or LSB-first data transmission.
7.  To use parity mode:
    a. Enable parity mode by writing 0x1 to the Frame Format field in the `CTRLA` register (`CTRLA.FORM`).
    b. Configure the Parity Mode bit in the `CTRLB` register (`CTRLB.PMODE`) for even or odd parity.
8.  Configure the number of stop bits in the Stop Bit Mode bit in the `CTRLB` register (`CTRLB.SBMODE`).
9.  When using an internal clock, write the Baud register (`BAUD`) to generate the desired baud rate.
10. Enable the transmitter and receiver by writing '1' to the Receiver Enable and Transmitter Enable bits in the `CTRLB` register (`CTRLB.RXEN` and `CTRLB.TXEN`).

#### 34.6.2.2 Enabling, Disabling, and Resetting
This peripheral is enabled by writing '1' to the Enable bit in the Control A register (`CTRLA.ENABLE`), and disabled by writing '0' to it.
Writing '1' to the Software Reset bit in the Control A register (`CTRLA.SWRST`) will reset all registers of this peripheral to their initial states, except the `DBGCTRL` register, and the peripheral is disabled.
Refer to the `CTRLA` register description for details.

#### 34.6.2.3 Clock Generation and Selection
For both synchronous and asynchronous modes, the clock used for shifting and sampling data can be generated internally by the SERCOM baud-rate generator or supplied externally through the XCK line.
The synchronous mode is selected by writing a '1' to the Communication Mode bit in the Control A register (`CTRLA.CMODE`), the asynchronous mode is selected by writing a zero to `CTRLA.CMODE`.
The internal clock source is selected by writing 0x1 to the Operation Mode bit field in the Control A register (`CTRLA.MODE`), the external clock source is selected by writing 0x0 to `CTRLA.MODE`.
The SERCOM baud-rate generator is configured as in the figure below.
In asynchronous mode (`CTRLA.CMODE=0`), the 16-bit Baud register value is used.
In synchronous mode (`CTRLA.CMODE=1`), the eight LSBs of the Baud register are used. Refer to Clock Generation - Baud-Rate Generator for details on configuring the baud rate.
Figure 34-3. Clock Generation
*(Block diagram content ignored)*

#### 34.6.2.3.1 Synchronous Clock Operation
In synchronous mode, the `CTRLA.MODE` bit field determines whether the transmission clock line (XCK) serves either as input or output. The dependency between clock edges, data sampling, and data change is the same for internal and external clocks. Data input on the RxD pin is sampled at the opposite XCK clock edge when data is driven on the TxD pin.
The Clock Polarity bit in the Control A register (`CTRLA.CPOL`) selects which XCK clock edge is used for RxD sampling, and which is used for TxD change:
When `CTRLA.CPOL` is '0', the data will be changed on the rising edge of XCK, and sampled on the falling edge of XCK.
When `CTRLA.CPOL` is '1', the data will be changed on the falling edge of XCK, and sampled on the rising edge of XCK.
Figure 34-4. Synchronous Mode XCK Timing
*(Figure content ignored)*
When the clock is provided through XCK (`CTRLA.MODE=0x0`), the shift registers operate directly on the XCK clock. This means that XCK is not synchronized with the system clock and, therefore, can operate at frequencies up to the system frequency.

#### 34.6.2.4 Data Transmission
The USART Transmit Data register (TXDATA) and USART Receive Data register (RXDATA) share the same I/O address, referred to as the Data register (DATA). Writing the DATA register will update the TXDATA register. Reading the DATA register will return the contents of the RXDATA register.

Data transmission is initiated by writing the data to be sent into the TX buffer by accessing the DATA register. Then, the data in TX buffer will be moved to the Shift register when the Shift register is empty and ready to send a new frame. After the Shift register is loaded with data, the data frame will be transmitted.
When the entire data frame including Stop bits have been transmitted (both the TX buffer and the shift register are empty) and no new data was written to the DATA register, the Transmit Complete Interrupt flag in the Interrupt Flag Status and Clear register (`INTFLAG.TXC`) will be set, and the optional interrupt will be generated.
The Data Register Empty flag in the Interrupt Flag Status and Clear register (`INTFLAG.DRE`) indicates that at least FIFO threshold (`CTRLC.TXTRHOLD`) locations are empty and ready for new data. The DATA register should only be written to when `INTFLAG.DRE` is set.

#### 34.6.2.5 Data Reception
The receiver accepts data when a valid Start bit is detected. Each bit following the Start bit will be sampled according to the baud rate or XCK clock, and shifted into the receive Shift register until the first Stop bit of a frame is received. The second Stop bit will be ignored by the receiver.
When the first Stop bit is received and a complete serial frame is present in the Receive Shift register, the contents of the Shift register will be moved into the receive buffer.
The Receive Complete Interrupt flag in the Interrupt Flag Status and Clear register (`INTFLAG.RXC`) will be set when the number of bytes present in the FIFO equals or is higher than the threshold value defined by the `CTRLC.RXTRHOLD` setting. An optional interrupt will be generated.
The received data can be read from the DATA register when the Receive Complete Interrupt flag is set.

#### 34.6.2.5.1 Disabling the Transmitter
The transmitter is disabled by writing '0' to the Transmitter Enable bit in the `CTRLB` register (`CTRLB.TXEN`). Disabling the transmitter will complete only after any ongoing and pending transmissions are completed, that is, there is no data in the Transmit Shift register and TX buffer to transmit.

#### 34.6.2.6 Disabling the Receiver
Writing '0' to the Receiver Enable bit in the `CTRLB` register (`CTRLB.RXEN`) will disable the receiver, flush the two-level receive buffer, and data from ongoing receptions will be lost.

#### 34.6.2.6.1 Error Bits
The USART receiver has three error bits in the Status (STATUS) register: Frame Error (FERR), Buffer Overflow (BUFOVF), and Parity Error (PERR). Once an error happens, the corresponding error bit will be set until it is cleared by writing '1' to it. These bits are also cleared automatically when the receiver is disabled.
There are two methods for buffer overflow notification, selected by the Immediate Buffer Overflow Notification bit in the Control A register (`CTRLA.IBON`):
When `CTRLA.IBON=1`, `STATUS.BUFOVF` is raised immediately upon buffer overflow. Software can then empty the receive FIFO by reading RXDATA, until the Receive Complete Interrupt flag (`INTFLAG.RXC`) is cleared.
When `CTRLA.IBON=0`, the Buffer Overflow condition is attending data through the receive FIFO. After the received data is read, `STATUS.BUFOVF` will be set along with `INTFLAG.RXC`.

#### 34.6.2.6.2 Asynchronous Data Reception
The USART includes a clock recovery and data recovery unit for handling asynchronous data reception.
The clock recovery logic can synchronize the incoming asynchronous serial frames at the RxD pin to the internally generated baud-rate clock.
The data recovery logic samples and applies a low-pass filter to each incoming bit, thereby improving the noise immunity of the receiver.

#### 34.6.2.6.3 Asynchronous Operational Range
The operational range of the asynchronous reception depends on the accuracy of the internal baud-rate clock, the rate of the incoming frames, and the frame size (in number of bits). In addition, the operational range of the receiver is depending on the difference between the received bit rate and the internally generated baud rate. If the baud rate of an external transmitter is too high or too low compared to the internally generated baud rate, the receiver will not be able to synchronize the frames to the start bit.
There are two possible sources for a mismatch in baud rate: First, the reference clock will always have some minor instability. Second, the baud-rate generator cannot always do an exact division of the reference clock frequency to get the baud rate desired. In this case, the BAUD register value should be set to give the lowest possible error. Refer to Clock Generation - Baud-Rate Generator for details.
Recommended maximum receiver baud-rate errors for various character sizes are shown in the table below.

Table 34-4. Asynchronous Receiver Error for 16-fold Oversampling
| D (Data bits+Parity) | R<sub>SLOW</sub> [%] | R<sub>FAST</sub> [%] | Max. total error [%] | Recommended max. Rx error [%] |
|----------------------|------------------|------------------|----------------------|-------------------------------|
| 5                    | 94.12            | 107.69           | +5.88/-7.69          | ±2.5                          |
| 6                    | 94.92            | 106.67           | +5.08/-6.67          | ±2.0                          |
| 7                    | 95.52            | 105.88           | +4.48/-5.88          | ±2.0                          |
| 8                    | 96.00            | 105.26           | +4.00/-5.26          | ±2.0                          |
| 9                    | 96.39            | 104.76           | +3.61/-4.76          | ±1.5                          |
| 10                   | 96.70            | 104.35           | +3.30/-4.35          | ±1.5                          |

The following equations calculate the ratio of the incoming data rate and internal receiver baud rate:
R<sub>SLOW</sub> = (D + 1)S / (S - 1 + D * S * S<sub>F</sub>)
R<sub>FAST</sub> = (D + 2)S / (S - 1 + D * S * S<sub>M</sub>)

*   R<sub>SLOW</sub> is the ratio of the slowest incoming data rate that can be accepted in relation to the receiver baud rate
*   R<sub>FAST</sub> is the ratio of the fastest incoming data rate that can be accepted in relation to the receiver baud rate
*   D is the sum of character size and parity size (D = 5 to 10 bits)
*   S is the number of samples per bit (S = 16, 8 or 3)
*   S<sub>F</sub> is the first sample number used for majority voting (S<sub>F</sub> = 7, 3, or 2) when `CTRLA.SAMPA=0`, S<sub>M</sub> is the middle sample number used for majority voting (S<sub>M</sub> = 8, 4, or 2) when `CTRLA.SAMPA=0`.
The recommended maximum Rx Error assumes that the receiver and transmitter equally divide the maximum total error. Its connection to the SERCOM Receiver error acceptance is depicted in this figure:
Figure 34-5. USART Rx Error Calculation
*(Figure content ignored)*
The recommendation values in the table above accommodate errors of the clock source and the baud generator. The following figure gives an example for a baud rate of 3Mbps:
Figure 34-6. USART Rx Error Calculation Example
*(Figure content ignored)*

### 34.6.3 Additional Features

#### 34.6.3.1 Parity
Even or odd parity can be selected for error checking by writing 0x1 to the Frame Format bit field in the Control A register (`CTRLA.FORM`).
If even parity is selected (`CTRLB.PMODE=0`), the parity bit of an outgoing frame is '1' if the data contains an odd number of bits that are '1', making the total number of '1' even.
If odd parity is selected (`CTRLB.PMODE=1`), the parity bit of an outgoing frame is '1' if the data contains an even number of bits that are '0', making the total number of '1' odd.
When parity checking is enabled, the parity checker calculates the parity of the data bits in incoming frames and compares the result with the parity bit of the corresponding frame. If a parity error is detected, the Parity Error bit in the Status register (`STATUS.PERR`) is set.

#### 34.6.3.2 Hardware Handshaking
The USART features an out-of-band hardware handshaking flow control mechanism, implemented by connecting the RTS and CTS pins with the remote device, as shown in the figure below.
Figure 34-7. Connection with a Remote Device for Hardware Handshaking
*(Figure content ignored)*
Hardware handshaking is only available in the following configuration:
*   USART with internal clock (`CTRLA.MODE=1`),
*   Asynchronous mode (`CTRLA.CMODE=0`),
*   and Flow control pinout (`CTRLA.TXPO=2`).

When the receiver is disabled or the receive FIFO is full, the receiver will drive the RTS pin high. This notifies the remote device to stop transfer after the ongoing transmission. Enabling and disabling the receiver by writing to `CTRLB.RXEN` will set/clear the RTS pin after a synchronization delay. When the receive FIFO goes full, RTS will be set immediately and the frame being received will be stored in the shift register until the receive FIFO is no longer full.
Figure 34-8. Receiver Behavior when Operating with Hardware Handshaking
*(Figure content ignored)*
The current CTS Status is in the STATUS register (`STATUS.CTS`). Character transmission will start only if `STATUS.CTS=0`. When CTS is set, the transmitter will complete the ongoing transmission and stop transmitting.
Figure 34-9. Transmitter Behavior when Operating with Hardware Handshaking
*(Figure content ignored)*

#### 34.6.3.3 IrDA Modulation and Demodulation
Transmission and reception can be encoded IrDA compliant up to 115.2 kb/s. IrDA modulation and demodulation work in the following configuration:
*   IrDA encoding enabled (`CTRLB.ENC=1`),
*   Asynchronous mode (`CTRLA.CMODE=0`),
*   and 16x sample rate (`CTRLA.SAMPR[0]=0`).

During transmission, each low bit is transmitted as a high pulse. The pulse width is 3/16 of the baud rate period, as illustrated in the figure below.
Figure 34-10. IrDA Transmit Encoding
*(Figure content ignored)*
The reception decoder has two main functions.
The first is to synchronize the incoming data to the IrDA baud rate counter. Synchronization is performed at the start of each zero pulse.
The second main function is to decode incoming Rx data. If a pulse width meets the minimum length set by configuration (`RXPL.RXPL`), it is accepted. When the baud rate counter reaches its middle value (1/2 bit length), it is transferred to the receiver.
**Note:** Note that the polarity of the transmitter and receiver are opposite: During transmission, a '0' bit is transmitted as a '1' pulse. During reception, an accepted '0' pulse is received as a '0' bit.
**Example:** The figure below illustrates reception where `RXPL.RXPL` is set to 19. This indicates that the pulse width should be at least 20 SE clock cycles. When using BAUD=0x666 or 160 SE cycles per bit, this corresponds to 2/16 baud clock as minimum pulse width required. In this case the first bit is accepted as a '0', the second bit as '1' and the third bit also '1'. A low pulse is rejected since it does not meet the minimum requirement of 2/16 baud clock.
Figure 34-11. IrDA Receive Decoding
*(Figure content ignored)*

#### 34.6.3.4 Break Character Detection and Auto-Baud/LIN Client
Break character detection and auto-baud are available in this configuration:
*   Auto-baud frame format (`CTRLA.FORM = 0x04` or `0x05`),
*   Asynchronous mode (`CTRLA.CMODE = 0`),
*   and 16x sample rate using fractional baud rate generation (`CTRLA.SAMPR = 1`).

The USART uses a break detection threshold of greater than 11 nominal bit times at the configured baud rate. At one time, more than 11 consecutive dominant bits are detected on the bus, the USART detects a Break field. When a Break field has been detected, the Receive Break interrupt flag (`INTFLAG.RXBRK`) is set and the USART expects the Sync Field character to be 0x55. This field used to update the actual baud rate in order to stay synchronized. If the received Sync character is not 0x55, then the Inconsistent Sync Field error flag (`STATUS.ISF`) is set along with the Error interrupt flag (`INTFLAG.ERROR`), and the baud rate is unchanged.
The auto-baud follows the LIN format. All LIN Frames start with a Break Field followed by a Sync Field.
Figure 34-12. LIN Break and Sync Fields
*(Figure content ignored)*
After a break field is detected and the start bit of the Sync Field is detected, a counter is started. The counter is then incremented for the next 8 bit times of the Sync Field. At the end of these 8 bit times, the counter is stopped. At this moment, the 13 most significant bits of the counter (value divided by 8) give the new clock divider (`BAUD.BAUD`), and the 3 least significant bits of this value (the remainder) give the new Fractional Part (`BAUD.FP`).
When the Sync Field has been received, the clock divider (`BAUD.BAUD`) and the Fractional Part (`BAUD.FP`) are updated after a synchronization delay. After the Break and Sync Fields are received, multiple characters of data can be received.

#### 34.6.3.5 LIN Host
LIN Host is available with the following configuration:
*   LIN Host format (`CTRLA.FORM = 0x02`)
*   Asynchronous mode (`CTRLA.CMODE = 0`)
*   16x sample rate using fractional Baud Rate Generation (`CTRLA.SAMPR = 1`)
*   LSB is transmitted first (`CTRLA.DORD = 1`)

LIN frames start with a header transmitted by the Host. The header consists of the break, sync, and identifier fields. After the Host transmits the header, the addressed client will respond with 1-8 bytes of data plus checksum.
Figure 34-13. LIN Frame Format
*(Figure content ignored)*
Using the LIN command field (`CTRLB.LINCMD`), the complete header can be automatically transmitted, or software can control transmission of the various header components.
When `CTRLB.LINCMD = 0x1`, software controls transmission of the LIN header. In this case, software uses the following sequence.
*   The `CTRLB.LINCMD` is written to 0x1.
*   The DATA register written to 0x00. This triggers transmission of the break field by hardware. Note that writing the DATA register with any other value will also result in the transmission of the break field by hardware.
*   The DATA register written to 0x55. The 0x55 value (sync) is transmitted.
*   The DATA register written to the identifier. The identifier is transmitted.
**Note:** It is recommended to use the Data Register Empty Flag in the Interrupt Flag Status and Clear register (`INTFLAG.DRE`) to ensure no extra delay is added during the transmission of the data.
When `CTRLB.LINCMD = 0x2`, hardware controls transmission of the LIN header. In this case, software uses the following sequence.
*   `CTRLB.LINCMD` is written to 0x2.
*   DATA register written to the identifier. This triggers transmission of the complete header by hardware. First the break field is transmitted. Next, the sync field is transmitted, and finally the identifier is transmitted.

In LIN host mode, the length of the break field is programmable using the break length field (`CTRLC.BRKLEN`). When the LIN header command is used (`CTRLB.LINCMD = 0x2`), the delay between the break and sync fields, in addition to the delay between the sync and ID fields are configurable using the header delay field (`CTRLC.HDRDLY`). When manual transmission is used (`CTRLB.LINCMD = 0x1`), software controls the delay between break and sync.
Figure 34-14. LIN Header Generation
*(Figure content ignored)*
After header transmission is complete, the client responds with 1-8 data bytes plus checksum.

#### 34.6.3.6 RS485
RS485 is available with the following configuration:
*   USART frame format (`CTRLA.FORM = 0x00` or `0x01`)
*   RS485 pinout (`CTRLA.TXPO = 0x3`).

The RS485 feature enables control of an external line driver as shown in the figure below. While operating in RS485 mode, the transmit enable pin (TE) is driven high when the transmitter is active.
Figure 34-15. RS485 Bus Connection
*(Figure content ignored)*
The TE pin will remain high for the complete frame including stop bits. If a Guard Time is programmed in the Control C register (`CTRLC.GTIME`), the line will remain driven after the last character completion. The following figure shows a transfer with one stop bit and `CTRLC.GTIME = 3`.
Figure 34-16. Example of TE Drive with Guard Time
*(Figure content ignored)*
The Transmit Complete interrupt flag (`INTFLAG.TXC`) will be raised after the guard time is complete and TE goes low.

#### 34.6.3.7 ISO 7816 for Smart Card Interfacing
The SERCOM USART features an ISO/IEC 7816-compatible operating mode. This mode permits interfacing with smart cards and Security Access Modules (SAM) communicating through an ISO 7816 link. Both T = 0 and T = 1 protocols defined by the ISO 7816 specification are supported.
ISO 7816 is available with the following configuration:
*   ISO 7816 format (`CTRLA.FORM = 0x07`)
*   Inverse transmission and reception (`CTRLA.RXINV = 1` and `CTRLA.TXINV = 1`)
*   Single bidirectional data line (`CTRLA.TXPO` and `CTRLA.RXPO` configured to use the same data pin)
*   Even parity (`CTRLB.PMODE = 0`)
*   8-bit character size (`CTRLB.CHSIZE = 0`)
*   T=0 (`CTRLA.CMODE=1`) or T=1 (`CTRLA.CMODE = 0`)

ISO 7816 is a half duplex communication on a single bidirectional line. The USART connects to a smart card as shown below. The output is only driven when the USART is transmitting. The USART is considered as the Host of the communication as it generates the clock.
Figure 34-17. Connection of a Smart Card to the SERCOM USART
*(Figure content ignored)*
ISO 7816 characters are specified as 8 bits with even parity. The USART must be configured accordingly.
The USART cannot operate concurrently in both receiver and transmitter modes as the communication is unidirectional. It has to be configured according to the required mode by enabling or disabling either the receiver or the transmitter as desired. Enabling both the receiver and the transmitter at the same time in ISO 7816 mode may lead to unpredictable results.
The ISO 7816 specification defines an inverse transmission format. Data bits of the character must be transmitted on the I/O line at their negative value (`CTRLA.RXINV = 1` and `CTRLA.TXINV = 1`).

**Protocol T=0**
In T = 0 protocol, a character is made up of these:
*   One start bit
*   Eight data bits
*   One parity bit
*   One guard time, which lasts two bit times

The transfer is synchronous (`CTRLA.CMODE = 1`). The transmitter shifts out the bits and does not drive the I/O line during the guard time. Additional guard time can be added by programming the Guard Time (`CTRLC.GTIME`).
If no parity error is detected, the I/O line remains during the guard time and the transmitter can continue with the transmission of the next character, as shown in the figure below.
Figure 34-18. T=0 Protocol without Parity Error
*(Figure content ignored)*
If a parity error is detected by the receiver, it drives the I/O line to 0 during the guard time, as shown in the next figure. This error bit is also named NACK, for Non Acknowledge. In this case, the character lasts 1 bit time more, as the guard time length is the same and is added to the error bit time, which lasts 1 bit time.
Figure 34-19. T=0 Protocol with Parity Error
*(Figure content ignored)*
When the USART is the receiver and it detects a parity error, the parity error bit in the Status Register (`STATUS.PERR`) is set and the character is not written to the receive FIFO.

**Receive Error Counter**
The receiver also records the total number of errors (receiver parity errors and NACKs from the remote transmitter) up to a maximum of 255. This can be read in the Receive Error Count (`RXERRCNT`) register. The `RXERRCNT` register is automatically cleared on read.

**Receive NACK Inhibit**
The receiver can also be configured to inhibit error generation. This can be achieved by setting the Inhibit Not Acknowledge (`CTRLC.INACK`) bit. If `CTRLC.INACK` is 1, no error signal is driven on the I/O line even if a parity error is detected. Moreover, if `CTRLC.INACK` is set, the erroneous received character is stored in the receive FIFO, and the `STATUS.PERR` bit is set. Inhibit not acknowledge (`CTRLC.INACK`) takes priority over disable successive receive NACK (`CTRLC.DSNACK`).

**Transmit Character Repetition**
When the USART is transmitting a character and gets a NACK, it can automatically repeat the character before moving on to the next character. Repetition is enabled by writing the Maximum Iterations register (`CTRLC.MAXITER`) to a non-zero value. The USART repeats the character the number of times specified in `CTRLC.MAXITER`.
When the USART repetition reaches the programmed value in `CTRLC.MAXITER`, the `STATUS.ITER` bit is set and the internal iteration counter is reset. If the repetition of the character is acknowledged by the receiver before the maximum iteration is reached, the repetitions are stopped and the iteration counter is cleared.

**Disable Successive Receive NACK**
The receiver can limit the number of successive NACKs sent back to the remote transmitter. This is programmed by setting the Disable Successive NACK bit (`CTRLC.DSNACK`). The maximum number of NACKs transmitted is programmed in the `CTRLC.MAXITER` field. As soon as the maximum is reached, the character is considered as correct, an acknowledge is sent on the line, the `STATUS.ITER` bit is set and the internal iteration counter is reset.

**Protocol T = 1**
When operating in ISO7816 protocol T = 1, the transmission is asynchronous (`CTRLA.CMODE = 0`) with one or two stop bits. After the stop bits are sent, the transmitter does not drive the I/O line.
Parity is generated when transmitting and checked when receiving. Parity FIFO error detection sets the `STATUS.PERR` bit, and the erroneous character is written to the receive FIFO. When using T=1 protocol, the receiver does not signal errors on the I/O line and the transmitter does not retransmit.

#### 34.6.3.8 Collision Detection
When the receiver and transmitter are connected either through pin configuration or externally, transmit collision can be detected after selecting the Collision Detection Enable bit in the `CTRLB` register (`CTRLB.COLDEN=1`). To detect collision, the receiver and transmitter must be enabled (`CTRLB.RXEN=1` and `CTRLB.TXEN=1`).
Collision detection is performed for each bit transmitted by comparing the received value with the transmit value, as shown in the figure below. While the transmitter is idle (no transmission in progress), characters can be received on RxD without triggering a collision.
Figure 34-20. Collision Checking
*(Figure content ignored)*
The next figure shows the conditions for a collision detection. In this case, the start bit and the first data bit are received with the same value as transmitted. The second received data bit is found to be different than the transmitted bit at the detection point, which indicates a collision.
Figure 34-21. Collision Detected
*(Figure content ignored)*
When a collision is detected, the USART follows this sequence:
1.  Abort the current transfer.
2.  Flush the transmit buffer.
3.  Disable transmitter (`CTRLB.TXEN=0`)
    *   This is done after a synchronization delay. The `CTRLB` Synchronization Busy bit (`SYNCBUSY.CTRLB`) will be set until this is complete.
    *   After disabling, the TxD pin will be tri-stated.
4.  Set the Collision Detected bit (`STATUS.COLL`) along with the Error interrupt flag (`INTFLAG.ERROR`).
5.  Set the Transmit Complete interrupt flag (`INTFLAG.TXC`), since the transmit buffer no longer contains data.

After a collision, software must manually enable the transmitter again before continuing, after assuring that the `CTRLB` Synchronization Busy bit (`SYNCBUSY.CTRLB`) is not set.

#### 34.6.3.9 Loop-Back Mode
For loop-back mode, configure the Receive Data Pinout (`CTRLA.RXPO`) and Transmit Data Pinout (`CTRLA.TXPO`) to use the same data pins for transmit and receive. The loop-back is through the pad, so the signal is also available externally.

#### 34.6.3.10 Start-of-Frame Detection
The USART start-of-frame detector can wake-up the CPU when it detects a Start bit. In Standby Sleep mode, an internal fast start-up oscillator must be selected as the `GCLK_SERCOMx_CORE` source. If the fast start-up oscillator is not selected, there may be corruption until the oscillator is stable.
When a 1-to-0 transition is detected on RxD, an internal fast start-up oscillator is powered up and the USART clock is enabled. After start-up, the rest of the data frame can be received, provided that the baud rate is slow enough in relation to the fast start-up internal oscillator start-up time. The start-up time of this oscillator varies with supply voltage and temperature.
The USART start-of-frame detection works both in Asynchronous and Synchronous modes. It is enabled by writing '1' to the Start of Frame Detection Enable bit in the Control B register (`CTRLB.SFDE`).
If the Receive Start Interrupt Enable bit in the Interrupt Enable Set register (`INTENSET.RXS`) is set, the Receive Start interrupt is generated immediately when a start is detected.
When using start-of-frame detection without the Receive Start interrupt, start detection will force the 8 MHz internal oscillator and USART clock active while the frame is being received. In this case, the CPU will not wake up until the receive complete interrupt is generated.

#### 34.6.3.11 Sample Adjustment
In asynchronous mode (`CTRLA.CMODE=0`), three samples in the middle are used to determine the value based on majority voting. The three samples used for voting can be selected using the Sample Adjustment bit field in Control A register (`CTRLA.SAMPA`). When `CTRLA.SAMPA=0`, samples 7-8-9 are used for 16x oversampling, and samples 3-4-5 are used for 8x oversampling.

#### 34.6.3.12 32-bit Extension
For better system bus utilization, 32-bit data receive and transmit can be enabled separately by writing to the Data 32-bit bit field in the Control C register (`CTRLC.DATA32B`). When enabled, writes and/or reads to the DATA register are 32 bit in size.
If frames are not multiples of 4 Bytes, the Length counter (`LENGTH.LEN`) and length enable (`LENGTH.LENEN`) must be configured before data transfer begins. `LENGTH.LEN` must be enabled only when `CTRLC.DATA32B` is enabled.
The figure below shows the order of transmit and receive when using 32-bit extension. Bytes are transmitted or received, and stored in order from 0 to 3. Only 8-bit and smaller character sizes are supported. If the character size is less than 8 bits, characters will still be 8-bit aligned within the 32-bit APB write or read. The unused bits within each byte will be zero for received data and unused for transmit data.
Figure 34-22. 32-bit Extension Ordering
*(Figure content ignored)*
A receive transaction using 32-bit extension is in the next figure. The Receive Complete flag (`INTFLAG.RXC`) is raised every four received Bytes. For transmit transactions, the Data Register Empty flag (`INTFLAG.DRE`) is raised instead of `INTFLAG.RXC`.
Figure 34-23. 32-bit Extension Receive Operation
*(Figure content ignored)*

**Data Length Configuration**
When the Data Length Enable bit field in the Length register (`LENGTH.LENEN`) is written to 0x1 or 0x2, the Data Length bit (`LENGTH.LEN`) determines the number of characters to be transmitted or received from 1 to 255.
**Note:** There is one internal length counter that can be used for either transmit (`LENGTH.LENEN=0x1`) or receive (`LENGTH.LENEN=0x2`), but not for both simultaneously.
The `LENGTH` register must be written before the frame begins. If `LENGTH.LEN` is not a multiple of 4 Bytes, the final `INTFLAG.RXC/DRE` interrupt will be raised when the last byte is received/sent. The internal length counter is reset when `LENGTH.LEN` is reached or when `LENGTH.LENEN` is written to 0x0.
Writing the `LENGTH` register while a frame is in progress will produce unpredictable results. If `LENGTH.LENEN` is not set and a frame is not a multiple of 4 Bytes, the remainder may be lost. Attempting to use the length counter for transmit and receive at the same time will produce unpredictable results.

#### 34.6.3.13 FIFO Operation
The USART embeds up to 16-bytes FIFO capability. The receive / transmit buffer is considered to have the FIFO mode enabled when the `FIFOEN` bit in `CTRLC` register is set to a '1' (`CTRLC.FIFOEN = 1`). By default, the FIFO can act as a 16-by-8-bit array, or as a 4-by-32-bit array, depending on the setting of the `CTRLC.DATA32B` bit.
The hardware around this array implements four pointers, called the CPU Write Pointer (CPUWRPTR), the CPU Read Pointer (CPURDPTR), the USART Write Pointer (USARTWRPTR) and the USART Read pointer (USARTRDPTR). All of these pointers reset to '0'. The CPUWRPTR and CPURDPTR pointers are native to the CPU clock domain, while the USARTWRPTR and USARTRDPTR are native to the USART domain. The location pointed to by the CPUWRPTR is the current TX FIFO. The location pointed to by the CPURDPTR becomes the current RX FIFO. Writes to DATA register by the CPU will point to TX FIFO. Reads to DATA register by the CPU will point to RX FIFO. The location pointed to by the USARTWRPTR / USARTRDPTR is logically the current RX/TX shift registers.
Figure 34-24. FIFO Overview
*(Block diagram content ignored)*
The interrupts and DMA triggers are generated according to FIFO threshold settings in Control C register (`CTRLC.TXTRHOLD`, `CTRLC.RXTRHOLD`).
The Data Register Empty interrupt flag, and the DMA TX trigger respectively, are generated when the available place in the TX FIFO is equal or higher than the threshold value defined by the `CTRLC.TXTRHOLD` settings. The Transmit Complete interrupt is generated when the TX FIFO is empty and the entire data (including the stop bits) has been transmitted.
The Receive Complete interrupt flag, and the DMA RX trigger respectively, are generated when the number of bytes present in the RX FIFO equals or is higher than the threshold value defined by the `CTRLC.RXTRHOLD` settings. The ERROR interrupt flag is generated when both RX shifter and the RX FIFO are full.
The FIFO is fully accessible if the SERCOM is halted, by writing the corresponding CPU FIFO pointer in the `FIFOPTR` register. The RX or TX FIFO can be individually cleared, by setting the respective FIFO Clear bit in the Control B register (`CTRLB.FIFOCLR`). The FIFO Clear must be written before data transfer begins. Writing the FIFO Clear bits while a frame is in progress will produce unpredictable results.

#### 34.6.3.13.1 Pointer Operation when Data Transmission
As in normal operation, data transmission is initiated by writing the data to be sent by accessing the DATA register. CPUWRPTR is incremented by 1 every time the CPU writes a word to the memory array.
The data in TX FIFO will be moved to the shift register when the shift register is empty and ready to send a new frame, and the USARTRDPTR is incremented by 1. After the shift register is loaded with data, the data frame will be transmitted.
As long as data are present in TX FIFO (`FIFOSPACE.TXSPACE != 0`), a new data will be automatically loaded in the TX shift register when the previous data transmission is completed. All pointers increment to their maximum value, dictated by `CTRLC.DATA32B` bit, and then rolls over to '0'.
Depending the TX FIFO Threshold settings (`CTRLC.TXTRHOLD`), Interrupt Flag Status and Clear register (`INTFLAG.DRE`) indicates that the register is empty and ready for new data.
If the USART is halted when debugging, the CPUWRPTR pointer can be accessed by writing the `CPUWRPTR` bits in `FIFOPTR` register (`FIFOPTR.CPUWRPTR`). These bits will not increment if a new data is written into the TX FIFO memory.

#### 34.6.3.13.2 Pointer Operation when Data Reception
As in normal operation, when the first stop bit is received and a complete serial frame is present in the receive shift register, the contents of the shift register will be moved into the RX FIFO, and the USARTWRPTR is incremented by one. Depending the RX FIFO Threshold settings (`CTRLC.RXTRHOLD`), the Receive Complete interrupt flag (`INTFLAG.RXC`) is set, and the data can be read from RX FIFO. When a DATA is read, the CPURDPTR is incremented. As long as data are present in RX FIFO (`FIFOSPACE.RXSPACE != 0`), the CPU can read these data by accessing the DATA register. All pointers increment to their maximum value, dictated by `CTRLC.DATA32B` bit, and then rolls over to '0'.
When both RX shifter and RX FIFO are full, the Buffer Overflow status bit is set (`STATUS.BUFOVF`) and optional ERROR interrupt is generated. The data will not be stored while `BUFOVF` is '1', effectively disabling the module until software reads RX FIFO.
If the USART is halted when debugging, the RX FIFO CPU read pointer can be accessed by writing the `CPURDPTR` bits in `FIFOPTR` register (`FIFOPTR.CPURDPTR`). These bits will not increment if a new data is read from the RX FIFO memory.

## 34.6.4 DMA, Interrupts and Events

Table 34-5. Module Request for SERCOM USART
| Condition                                                                                | Request DMA | Request Interrupt | Request Event |
|------------------------------------------------------------------------------------------|-------------|-------------------|---------------|
| Standard (DRE): Data Register Empty                                                      | Yes         | Yes               | NA            |
| FIFO (DRE): at least TXTRHOLD locations in TX FIFO are empty                             | Yes (request cleared when data is written) | Yes               | NA            |
| Standard (RXC): Receive Complete                                                         | Yes         | Yes               | Yes           |
| FIFO (RXC): at least RXTRHOLD data available in RX FIFO, or a last word available and length frame reception completed. | Yes (request cleared when data is read) | Yes               | Yes           |
| Standard (TXC): Transmit Complete                                                        | NA          | Yes               | Yes           |
| FIFO (TXC): Transmit Complete and TX FIFO is empty                                       | NA          | Yes               | Yes           |
| Receive Start (RXS)                                                                      | NA          | Yes               | Yes           |
| Clear to Send Input Change (CTSIC)                                                       | NA          | Yes               | Yes           |
| Receive Break (RXBRK)                                                                    | NA          | Yes               | Yes           |
| Error (ERROR)                                                                            | NA          | Yes               | Yes           |

### 34.6.4.1 DMA Operation
The USART generates the following DMA requests:
*   Data received (RX): The request is set when data is available in the receive FIFO or if at least RXTRHOLD data are available in the RX FIFO when FIFO operation is enabled. The request is cleared when DATA is read.
*   Data transmit (TX): The request is set when the transmit buffer (TX DATA) is empty or if at least TXTRHOLD data locations are empty in the TX FIFO, when FIFO operation is enabled. The request is cleared when DATA is written.

### 34.6.4.2 Interrupts
The USART has the following interrupt sources. These are asynchronous interrupts, and can wake up the device from any sleep mode:
*   Data Register Empty (DRE)
*   Receive Complete (RXC)
*   Transmit Complete (TXC)
*   Receive Start (RXS)
*   Clear to Send Input Change (CTSIC)
*   Received Break (RXBRK)
*   Error (ERROR)

Each interrupt source has its own interrupt flag. The interrupt flag in the Interrupt Flag Status and Clear register (`INTFLAG`) will be set when the interrupt condition is met. Each interrupt can be individually enabled by writing '1' to the corresponding bit in the Interrupt Enable Set register (`INTENSET`), and disabled by writing '1' to the corresponding bit in the Interrupt Enable Clear register (`INTENCLR`).
An interrupt request is generated when the interrupt flag is set and if the corresponding interrupt is enabled. The interrupt request remains active until either the interrupt flag is cleared, the interrupt is disabled, or the USART is reset. For details on clearing interrupt flags, refer to the `INTFLAG` register description.
The value of `INTFLAG` indicates which interrupt is executed. Note that interrupts must be globally enabled for interrupt requests. Refer to Nested Vector Interrupt Controller for details.

### 34.6.5 Sleep Mode Operation
The behavior in sleep mode is depending on the clock source and the Run In Standby bit in the Control A register (`CTRLA.RUNSTDBY`):
*   Internal clocking, `CTRLA.RUNSTDBY=1`: `GCLK_SERCOMx_CORE` can be enabled in all sleep modes. Any interrupt can wake up the device.
*   External clocking, `CTRLA.RUNSTDBY=1`: The Receive Complete interrupt(s) can wake up the device.
*   Internal clocking, `CTRLA.RUNSTDBY=0`: Internal clock will be disabled, after any ongoing transfer was completed. The Receive Complete interrupt(s) can wake up the device.
*   External clocking, `CTRLA.RUNSTDBY=0`: External clock will be disconnected, after any ongoing transfer was completed. All reception will be dropped.

### 34.6.6 Synchronization
Some registers (or bit fields within a register) require synchronization when read and/or written.
Synchronization is denoted by the "Read-Synchronized" (or "Read-Synchronized Bits") and/or "Write-Synchronized" (or "Write-Synchronized Bits") property in each individual register description.
For more details, refer to Register Synchronization.

### 34.6.7 Debug Operation
When the CPU is halted in Debug mode, this peripheral will continue normal operation. If the peripheral is configured to require periodical service by the CPU through interrupts or similar, improper operation or data loss may result during debugging. This peripheral can be forced to halt operation during debugging. Refer to the Debug Control (`DBGCTRL`) register for details.

## 34.7 Register Summary
Refer to the Registers Description section for more details on register properties and access permissions.

| Offset | Name      | Bit Pos. | 7         | 6         | 5         | 4         | 3         | 2         | 1         | 0         |
|--------|-----------|----------|-----------|-----------|-----------|-----------|-----------|-----------|-----------|-----------|
| 0x00   | CTRLA     | 31:24    | DORD      | CPOL      | CMODE     | SAMPA[1:0]| RXPO[1:0] | FORM[3:0]             |           |
|        |           | 23:16    |           |           |           |           |           |           |           |           |
|        |           | 15:8     | RUNSTDBY  |           |           |           | MODE[2:0] | IBON      | TXINV     | RXINV     |
|        |           | 7:0      |           |           |           |           |           | ENABLE    | SWRST     | LINCMD[1:0]|
| 0x04   | CTRLB     | 23:16    | FIFOCLR[1:0]| PMODE   |           | ENC       | SFDE      | COLDEN    | RXEN      | TXEN      |
|        |           | 15:8     | SBMODE    |           |           |           | CHSIZE[2:0]|           |           |
|        |           | 7:0      | TXTRHOLD[1:0]|          | FIFOEN    |           |           |           |           |
| 0x08   | CTRLC     | 23:16    | MAXITER[2:0]|          | HDRDLY[1:0]|           | BRKLEN[1:0]| INACK     | DSNACK    |
|        |           | 15:8     |           |           |           |           |           | GTIME[2:0]|           |
|        |           | 7:0      | DATA32B[1:0]|          |           |           |           |           |           |
| 0x0C   | BAUD      | 15:8     | BAUD[15:8]|           |           |           |           |           |           |
|        |           | 7:0      | BAUD[7:0] |           |           |           |           |           |           |
| 0x0E   | RXPL      | 7:0      | RXPL[7:0] |           |           |           |           |           |           |
| 0x0F   | Reserved  |          |           |           |           |           |           |           |           |
| 0x13   | INTENCLR  | 7:0      | ERROR     | RXBRK     | CTSIC     | RXS       | RXC       | TXC       | DRE       |
| 0x14   | INTENSET  | 7:0      | ERROR     | RXBRK     | CTSIC     | RXS       | RXC       | TXC       | DRE       |
| 0x15   | Reserved  |          |           |           |           |           |           |           |           |
| 0x17   | INTFLAG   | 7:0      | ERROR     | RXBRK     | CTSIC     | RXS       | RXC       | TXC       | DRE       |
| 0x18   | STATUS    | 15:8     | ITER      | TXE       | COLL      | ISF       | CTS       | BUFOVF    | FERR      | PERR      |
| 0x19   |           | 7:0      |           |           |           |           |           |           |           |
| 0x1A   | SYNCBUSY  | 31:24    |           |           |           |           |           |           |           |
|        |           | 15:8     | LENGTH    | RXERRCNT  | CTRLB     | ENABLE    | SWRST     |           |           |
|        |           | 7:0      |           |           |           |           |           |           |           |
| 0x1C   | RXERRCNT  | 7:0      | RXERRCNT[7:0]|         |           |           |           |           |           |
| 0x20   | Reserved  |          |           |           |           |           |           |           |           |
| 0x21   | LENGTH    | 15:8     | LEN[7:0]  |           |           |           |           | LENEN[1:0]|           |
| 0x22   |           | 7:0      |           |           |           |           |           |           |           |
| ...    | ...       | ...      | ...       | ...       | ...       | ...       | ...       | ...       | ...       | ...       |
| 0x27   | Reserved  |          |           |           |           |           |           |           |           |
| 0x28   | DATA      | 31:24    | DATA[31:24]|          |           |           |           |           |           |
|        |           | 23:16    | DATA[23:16]|          |           |           |           |           |           |
|        |           | 15:8     | DATA[15:8]|           |           |           |           |           |           |
|        |           | 7:0      | DATA[7:0] |           |           |           |           |           |           |
| 0x2C   | Reserved  |          |           |           |           |           |           |           |           |
| 0x2F   | DBGCTRL   | 7:0      |           |           |           |           |           |           | DBGSTOP   |
| 0x30   | Reserved  |          |           |           |           |           |           |           |           |
| 0x31   | FIFOSPACE | 15:8     | RXSPACE[4:0]|          | TXSPACE[4:0]|          |           |           |           |
| 0x33   |           | 7:0      |           |           |           |           |           |           |           |
| 0x34   | FIFOPTR   | 15:8     | CPURDPTR[3:0]|         | CPUWRPTR[3:0]|         |           |           |           |
| 0x36   |           | 7:0      |           |           |           |           |           |           |           |