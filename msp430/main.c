#include <msp430.h>
#include <string.h>

#include "config.h"
#include "dcxo.h"
#include "eeram.h"
#include "gps.h"
#include "i2c.h"
#include "math.h"
#include "pid.h"
#include "pps.h"
#include "tcxo.h"
#include "uart.h"

#define TIMER_NS (5) // 5 ns
#define TEMP_PPS_TOL (250) // 250ns

// internal state
int16_t curTempC = 0;
int32_t prevTempComp = TCXO_ERR;
int32_t tempComp = 0;
int32_t pidComp = 0;

// stat accumulators
int64_t accAvgError = 0;
uint64_t accRmsError = 0;

void doTrackingUpdate(int16_t ppsDelay);

void SysBoot();
void SysClk();
void processRequest(uint8_t len);

int main() {
    // bootstrap MCU
    SysBoot();
    I2C_init();
    DCXO_init();
    SysClk();
    _enable_interrupts();

    // init code modules
    EERAM_init();
    SysConf_load();
    PPS_init();
    PID_init();

    // pet watchdog
    WDTCTL |= WDTPW | WDTCNTCL;
    // release watchdog
    WDTCTL = WDTPW | (WDTCTL & 0x7F);

    // main processing loop
    for(;;) {
        // pet watchdog
        WDTCTL |= WDTPW | WDTCNTCL;
        // poll GPS data
        GPS_poll();
        // poll UART interface
        uint8_t len = UART_poll();
        if(len) processRequest(len);
        // check for PPS measurement
        int16_t ppsDelay = PPS_poll();
        if(ppsDelay != PPS_IDLE) {
            doTrackingUpdate(ppsDelay);
        }
    }

    return 0;
}

void doTrackingUpdate(int16_t ppsDelay) {
    // get DCXO temperature
    curTempC = DCXO_getTemp();
    uint8_t allowTempUpdate = (curTempC != DCXO_TMPS_ERR);
    if(allowTempUpdate) {
        // get temperature compensation
        tempComp = TCXO_getCompensation(curTempC);
        if(tempComp == TCXO_ERR) {
            if(prevTempComp == TCXO_ERR)
                tempComp = 0;
            else
                tempComp = prevTempComp;
        } else {
            if(prevTempComp == TCXO_ERR)
                PID_clearIntegral();
            prevTempComp = tempComp;
        }
    }

    // update PPS tracking loop
    if(ppsDelay != PPS_NOLOCK) {
        // TODO get actual PPS error
        int32_t ppsError = sysConf.ppsOffset;
        ppsError += mult16s16s(ppsDelay, TIMER_NS);
        // Update PID tracking loop
        pidComp = PID_update(ppsError);
        // Update average error (63s EMA)
        accAvgError -= accAvgError >> 5u;
        accAvgError += ppsError << 16u;
        // Update RMS error (63s EMA)
        accRmsError -= accRmsError >> 5u;
        accRmsError += (uint64_t)mult32s32s(ppsError, ppsError) << 16u;
        // wait for loop to settle before updating temperature compensation
        if(ppsError < -TEMP_PPS_TOL) allowTempUpdate = 0;
        if(ppsError >  TEMP_PPS_TOL) allowTempUpdate = 0;
    } else {
        allowTempUpdate = 0;
    }

    // combine compensation values
    int32_t totalComp = pidComp + tempComp;
    // apply frequency adjustment
    DCXO_setOffset(totalComp);

    if(!allowTempUpdate) return;
    // update temperature compensation
    TCXO_updateCompensation(curTempC, totalComp);
}

void SysBoot() {
    // Stop watchdog timer
    WDTCTL = WDTPW | WDTHOLD;
    // Switch FLL reference to REFO
    UCSCTL3 = SELREF__REFOCLK | FLLREFDIV__1;
    // clear pin assignments
    PASEL = 0; PBSEL = 0;
    PADIR = 0; PBDIR = 0;
    PAOUT = 0; PBOUT = 0;
    // enable pull-down on all pins
    PAREN = -1; PBREN = -1;
}

void SysClk() {
    // change XT1 to HF bypass mode
    UCSCTL6 = XT1BYPASS | XTS;
    // select XT1 function on pin
    PJSEL = BIT5;
    // set all dividers to 1
    UCSCTL5 = 0;
    // set ACLK to REFO, other clocks to XT1
    UCSCTL4 = SELA__REFOCLK | SELS__XT1CLK | SELM__XT1CLK;
    // disable DCO and FLL
    __bis_SR_register(SCG0);
    __bis_SR_register(SCG1);
    // set I2C clk to 400 kHz
    I2C_prescaler(6250u);
    // configure watchdog with 1s interval
    WDTCTL = WDTPW | WDTHOLD | WDTSSEL__ACLK | WDTCNTCL | WDTIS__32K;
}

// UART request strings
#define FLASH __attribute__ ((section(".text.const")))
// Status request
FLASH const char CMD_STATUS[] = "get status";
// get pps offset
FLASH const char CMD_GET_PPS_OFFSET[] = "get pps offset ";
// set pps offset
FLASH const char CMD_SET_PPS_OFFSET[] = "set pps offset ";

// request invalid
FLASH const char MSG_INVALID[] = "invalid";
// request ok
FLASH const char MSG_ACK[] = "ack";
// request fail
FLASH const char MSG_NACK[] = "nack";

// request handlers
void processGetStatus();
void processSetPpsOffset();

void processRequest(uint8_t len) {
    const char *cmd = UART_getMessage();

    if(strcmp(cmd, CMD_STATUS) == 0) {
        processGetStatus();
        return;
    }

    if(strncmp(cmd, CMD_SET_PPS_OFFSET, strlen(CMD_SET_PPS_OFFSET)) == 0) {
        if(len == (strlen(CMD_SET_PPS_OFFSET) + 4)) {
            processSetPpsOffset();
            return;
        }
    }

    // report use of invalid request
    strcpy(UART_getMessage(), MSG_INVALID);
    UART_send();
}

void processGetStatus() {
    char *msg = UART_getMessage();

    UART_send();
}

void processSetPpsOffset() {
    char *data = UART_getMessage() + strlen(CMD_SET_PPS_OFFSET);

    sysConf.ppsOffset = fromHex16(data);

    // acknowledge request
    strcpy(UART_getMessage(), MSG_ACK);
    UART_send();
}
