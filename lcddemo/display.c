#include "display.h"

#include "spi.h"

#include <msp430.h>

void display_init(void)
{
    spi_init();

    DISPLAY_DIR |= DISPLAY_RES | DISPLAY_SCE | DISPLAY_MODE;

    // Release USI for operation.
    USICTL0 &= ~USISWRST;

    // Initialization sequence.

    // Address slave chip.
    DISPLAY_OUT &= ~DISPLAY_SCE;

    // Reset.
    DISPLAY_OUT |= DISPLAY_RES;
    DISPLAY_OUT &= ~DISPLAY_RES;
    DISPLAY_OUT |= DISPLAY_RES;

    // Activate chip. Vertical addressing. Use basic instruction set.
    DISPLAY_SET_CMD();
    display_send_byte(0x21);
    display_send_byte(0x80 | 0x38);
    display_send_byte(0x04);
    display_send_byte(0x10 | 0x04);
    display_send_byte(0x20);
    display_send_byte(0x0c);
    //display_send_byte(0x0d);

    display_clear();
}

void display_goto(unsigned char row, unsigned char col)
{
    DISPLAY_SET_CMD();
    DISPLAY_GOTO_ROW(row);
    DISPLAY_GOTO_COL(col);
}

void display_clear(void)
{
    // Set x, y.
    display_goto(0, 0);

    // Clear everything.
    DISPLAY_SET_DATA();
    unsigned int i = 0;
    while(i++ < 6 * 84)
        display_send_byte(0x00);
}

void display_send_byte(unsigned char byte)
{
    DISPLAY_START_TRANSMIT();

    spi_send_byte(byte);

    DISPLAY_END_TRANSMIT();
}
