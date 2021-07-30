/******************************************************************************/
/*  Communication with an EEPROM (e.g. 2465) via I2C bus                      */
/*  The I2C module of the MSP430F2274 is used to communicate with the EEPROM. */
/*  The "Byte Write", "Page Write", "Current Address Read", "Random Read",    */
/*  "Sequential Read" and "Acknowledge Polling" commands or the EEPROM are    */
/*  realized.                                                                 */
/*                                                                            */
/*  developed with IAR Embedded Workbench V5.20.1                             */
/*                                                                            */
/*  Texas Instruments                                                         */
/*  William Goh                                                               */
/*  March 2011                                                                */
/*----------------------------------------------------------------------------*/
/*  updates                                                                   */
/*    Jan 2005:                                                               */
/*        - updated initialization sequence                                   */
/*    March 2009:                                                             */
/*        - updated code for 2xx USCI module                                  */
/*        - added Page Write and Sequential Read functions                    */
/*    March 2011:                                                             */
/*        - Fixed Random and Sequential Reads to Restart                      */
/*        - Added Page Write to support greater than 256 bytes                */
/*******************************************************************************
;
; THIS PROGRAM IS PROVIDED "AS IS". TI MAKES NO WARRANTIES OR
; REPRESENTATIONS, EITHER EXPRESS, IMPLIED OR STATUTORY,
; INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
; FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
; COMPLETENESS OF RESPONSES, RESULTS AND LACK OF NEGLIGENCE.
; TI DISCLAIMS ANY WARRANTY OF TITLE, QUIET ENJOYMENT, QUIET
; POSSESSION, AND NON-INFRINGEMENT OF ANY THIRD PARTY
; INTELLECTUAL PROPERTY RIGHTS WITH REGARD TO THE PROGRAM OR
; YOUR USE OF THE PROGRAM.
;
; IN NO EVENT SHALL TI BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
; CONSEQUENTIAL OR INDIRECT DAMAGES, HOWEVER CAUSED, ON ANY
; THEORY OF LIABILITY AND WHETHER OR NOT TI HAS BEEN ADVISED
; OF THE POSSIBILITY OF SUCH DAMAGES, ARISING IN ANY WAY OUT
; OF THIS AGREEMENT, THE PROGRAM, OR YOUR USE OF THE PROGRAM.
; EXCLUDED DAMAGES INCLUDE, BUT ARE NOT LIMITED TO, COST OF
; REMOVAL OR REINSTALLATION, COMPUTER TIME, LABOR COSTS, LOSS
; OF GOODWILL, LOSS OF PROFITS, LOSS OF SAVINGS, OR LOSS OF
; USE OR INTERRUPTION OF BUSINESS. IN NO EVENT WILL TI'S
; AGGREGATE LIABILITY UNDER THIS AGREEMENT OR ARISING OUT OF
; YOUR USE OF THE PROGRAM EXCEED FIVE HUNDRED DOLLARS
; (U.S.$500).
;
; Unless otherwise stated, the Program written and copyrighted
; by Texas Instruments is distributed as "freeware".  You may,
; only under TI's copyright in the Program, use and modify the
; Program without any charge or restriction.  You may
; distribute to third parties, provided that you transfer a
; copy of this license to the third party and the third party
; agrees to these terms by its first use of the Program. You
; must reproduce the copyright notice and any other legend of
; ownership on each copy or partial copy, of the Program.
;
; You acknowledge and agree that the Program contains
; copyrighted material, trade secrets and other TI proprietary
; information and is protected by copyright laws,
; international copyright treaties, and trade secret laws, as
; well as other intellectual property laws.  To protect TI's
; rights in the Program, you agree not to decompile, reverse
; engineer, disassemble or otherwise translate any object code
; versions of the Program to a human-readable form.  You agree
; that in no event will you alter, remove or destroy any
; copyright notice included in the Program.  TI reserves all
; rights not specifically granted under this license. Except
; as specifically provided herein, nothing in this agreement
; shall be construed as conferring by implication, estoppel,
; or otherwise, upon you, any license or other right under any
; TI patents, copyrights or trade secrets.
;
; You may not use the Program in non-TI devices.
;
******************************************************************************/

#define I2C_PORT_SEL  P3SEL
#define I2C_PORT_OUT  P3OUT
#define I2C_PORT_REN  P3REN
#define I2C_PORT_DIR  P3DIR
#define SDA_PIN       BIT1                  // UCB0SDA pin
#define SCL_PIN       BIT2                  // UCB0SCL pin
#define SCL_CLOCK_DIV 0x12                  // SCL clock devider

void InitI2C(unsigned char eeprom_i2c_address);
void EEPROM_ByteWrite(unsigned int Address , unsigned char Data);
void EEPROM_PageWrite(unsigned int StartAddress , unsigned char * Data , unsigned int Size);
unsigned char EEPROM_RandomRead(unsigned int Address);
unsigned char EEPROM_CurrentAddressRead(void);
void EEPROM_SequentialRead(unsigned int Address , unsigned char * Data , unsigned int Size);
void EEPROM_AckPolling(void);
