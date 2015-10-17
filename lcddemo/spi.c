#include "spi.h"

#include <msp430.h>

#include "defines.h"

void spi_init(void)
{
    // Configure ports for SPI.
    P1SEL |= DISPLAY_SCLK | DISPLAY_SDOUT | DISPLAY_SDIN;

    // Initialize USI module (for SPI).
    USICTL0 |= USISWRST;

    // Data output enable.
    // Master mode. MSB first. Enable clock, data in, data out.
    USICTL0 |= USIOE | USIMST | USIPE5 | USIPE6 | USIPE7;

    // No need for interrupts. Everything done with polling.

    // Serial data sampled on positive edge of SCLK.
    // Main clock used.
    // No need to divide clock. Display can handle upto 4 MHz clock.
    USICKCTL |= USICKPL | USISSEL1;
}

void spi_send_byte(unsigned char byte)
{
    // Put data into tx register.
    USISRL = byte;

    // 8-bit shift register mode.
    // USI bit count is 8.
    USICNT = 8;

    // Wait for transmission to finish.
    while(!(USICTL1 & USIIFG))
        ;
}
