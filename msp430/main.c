#include <msp430.h>
#include "tcxo.h"

int main() {
    TCXO_getCompensation(1234);
    TCXO_updateCompensation(2345, 567890);

    // TODO remove demo code
    WDTCTL = WDTPW | WDTHOLD;               // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings
    P1DIR |= 0x01;                          // Set P1.0 to output direction

    for(;;) {
        volatile unsigned int i;            // volatile to prevent optimization

        P1OUT ^= 0x01;                      // Toggle P1.0 using exclusive-OR

        i = 10000;                          // SW Delay
        do i--;
        while(i != 0);
    }

    // TODO bootsrap MCU
    return 0;
}
