#include <msp430.h>
#include <intrinsics.h>

#define eint() __eint()
#define dint() __dint()

// Timer interrupt. Blink led every half second.
static void __attribute__ ((__interrupt__(TIMERA1_VECTOR))) blink_led(void)
{
    // Reset overflow interrupt. Reading TAIV resets highest pending
    // interrupt flag for timers (TACCR1 CCIFG, TACCR2 CCIFG, TAIFG).
    // Not needed for TACCR0 CCIFG (TIMERA0_VECTOR).
    unsigned int ta = TAIV;
    (void)ta;

    // Toggle led.
    P1OUT ^= (1 << 6);
}

int main(void)
{
    // Disable watchdog timer.
    WDTCTL = WDTPW | WDTHOLD;

    // Calibrate main clock to 1Mhz.
    if(CALBC1_1MHZ == 0xff || CALDCO_1MHZ == 0xff)
        while(1); // Trap if calibration values were erased.
    DCOCTL = 0; // Choose lowest DCO clock and MODx values.
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;

    // Set timer in up mode to count to give 500 ms interrupt.

    // Use cpu clock for timer.
    TACTL |= TASSEL1;
    // Divide input clock by 8.
    TACTL |= (ID1 | ID0);
    // Count up.
    TACTL |= MC0;
    // Enable Timer A TAIFG interrupt.
    TACTL |= TAIE;
    // Enable global interrupt.
    eint();
    // Count up to 62500 => 0.5 second interrupt hit.
    TACCR0 = 62500;

    // Set to output so we can turn on led.
    P1DIR |= (1 << 6);

    while(1);
    
    return 0;
}
