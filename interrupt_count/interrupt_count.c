#include <msp430.h>
#include <intrinsics.h>

#define eint() __eint()
#define dint() __dint()

#define LCD_DIR P1DIR
#define LCD_OUT P1OUT
#define LCD_RS  (1 << 5)
#define LCD_E   (1 << 4)
#define BUTTON  (1 << 3)

// Assumes n > 14.
// Assumes a 1 Mhz clock.
// Can be 3 cycles too long.
static void delay_us(register unsigned int n)
{
    __asm__ __volatile__ (
        "   sub #11, %[n]  \n\t"    // 2 + 5 + 2 + 1 + 1 + 4 * k cycles.
                                    // k = (n - 11) / 4
        "   rra %[n]       \n\t"    // 2 for moving into %[n] before call.
                                    // 5 for calling function.
                                    // 3 for ret from function is not
                                    // included because division by 4
                                    // can make the number of cycles 3 short
                                    // in some cases. It's better to have
                                    // three more cycles in some cases
                                    // than come up short.
        "   rra %[n]       \n\t"
        "1: dec %[n]       \n\t"    // Have to use local labels (numbers).
        "   nop            \n\t"    // Nop (single cycle, single byte).
        "   jne 1b           \n"    // Jump backwards.
        : [n] "+r" (n));
}

// Assumes n > 0.
// Assumes a 1 Mhz clock.
// Not exactly n ms because 2 cycles needed to load %[n], 5 cycles to
// call function and 3 cycles to return from function.
static void delay_ms(register unsigned int n)
{
    __asm__ __volatile__ (
        "1: mov #331, r14  \n\t"   // 2 + 3 * 331 + 2 + 1 + 2 = 1000 cycles.
        "2: dec r14        \n\t"   // Have to use local labels (numbers).
        "   jne 2b         \n\t"   // Jump backwards.
        "   jmp $+2        \n\t"   // Two cycle single byte nop.
        "   dec %[n]       \n\t"   // Delay n 1ms cycles.
        "   jne 1b           \n"   // Jump backwards.
        : [n] "+r" (n));
}

#define LCD_SET_INSTRUCTION() LCD_OUT &= ~LCD_RS
#define LCD_SET_DATA()        LCD_OUT |= LCD_RS

void lcd_initialize(void);
void lcd_write_nibble(unsigned char data);
void lcd_write_byte(unsigned char data);
void lcd_send_instruction(unsigned char inst);
void lcd_send_data(unsigned char inst);
void lcd_goto(unsigned char loc);

volatile int count = 0;
// Display zero to begin.
volatile unsigned char counted = 1;

// Button interrupt. Increment global count here.
static void __attribute__ ((__interrupt__(PORT1_VECTOR))) count_press(void)
{
    // Need to manually clear P1IFG.
    P1IFG &= ~BUTTON;
    // Count how many times the button is pressed.
    ++count;
    // Indicates that the display needs to be updated.
    counted = 1;
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

    // Lcd initialization.
    lcd_initialize();

    // Set P1.3 as input to detect button press.
    P1DIR &= ~BUTTON;
    // Choose pullup.
    P1OUT |= BUTTON;
    // Enable pullup/pulldown resistor.
    P1REN |= BUTTON;
    // Button press detected on high to low transition.
    P1IES |= BUTTON;
    // Enable interrupt on P1.3.
    P1IE |= BUTTON;
    // Clear existing interrupts for button.
    P1IFG &= ~BUTTON;
    // Enable global interrupt.
    eint();

    while(1)
    {
        if(!counted)
            continue; // Don't update count if not necessary.

        // Critical section.
        P1IE &= ~BUTTON;

        // Debounce button press.
        delay_ms(20);

        // Wait until button is no longer pressed.
        // Otherwise, P1.3 will be low affecting lcd display when it
        // becomes an output.
        while(!(P1IN & BUTTON))
            ;

        // Disable pullup/pulldown resistor.
        P1REN &= ~BUTTON;
        // Need to make P1.3 an output port for LCD code to work.
        P1DIR |= BUTTON;

        // Show current count.

        // Convert number to string.
        char buf[10];
        buf[9] = '\0';
        char *p = buf + 8;
        int i = count;
        if(!i)
            *p-- = '0';
        while(i)
        {
            *p-- = (i % 10) + '0';
            i /= 10;
        }

        // Display string on lcd.
        lcd_goto(0x0); // Start on first row, first column.
        lcd_send_instruction(0x01); // Clear lcd.
        LCD_SET_DATA();
        while(*++p)
            lcd_send_data(*p);

        // Go back to using P1.3 as input and set it to interrupt.

        // Set P1.3 as input to detect button press.
        P1DIR &= ~BUTTON;
        // Choose pullup.
        P1OUT |= BUTTON;
        // Enable pullup/pulldown resistor.
        P1REN |= BUTTON;
        // Clear interrupt flag for button before re-enabling interrupts.
        // Needed because changing PxOUT, PxDIR can cause it to be set.
        P1IFG &= ~BUTTON;

        // Acknowledged increase in count.
        counted = 0;

        // End of critical section.
        P1IE |= BUTTON;
    }
    
    return 0;
}

// Initialization sequence for 4-bit access from HD44780 datasheet.
void lcd_initialize(void)
{
    LCD_DIR |= (LCD_RS | LCD_E | 0x0f);
    LCD_OUT &= ~LCD_RS;

    delay_ms(50);
    lcd_write_nibble(0x3);
    delay_ms(5);
    lcd_write_nibble(0x3);
    delay_us(200);
    lcd_write_nibble(0x3);
    delay_us(37);
    lcd_write_nibble(0x2);
    delay_us(37);
    lcd_send_instruction(0x28);
    lcd_send_instruction(0x08);
    lcd_send_instruction(0x01);
    lcd_send_instruction(0x06);

    lcd_send_instruction(0x0c); // Display on, cursor off, blinking off.
    lcd_send_instruction(0x02); // Go home.
    delay_us(1520 - 37);        // Going home takes longer than other
                                // instructions.
}

// Write a byte to HD44780.
void lcd_write_byte(unsigned char data)
{
    lcd_write_nibble(data >> 4);
    lcd_write_nibble(data & 0x0f);
}

// Assume data <= 0xf.
void lcd_write_nibble(unsigned char data)
{
    LCD_OUT |= LCD_E;
    LCD_OUT &= 0xf0;
    LCD_OUT |= data;
    delay_ms(1);
    LCD_OUT &= ~LCD_E;
    delay_ms(1);
}

// Use LCD_SET_INSTRUCTION() first.
void lcd_send_instruction(unsigned char inst)
{
    lcd_write_byte(inst);
    delay_us(37);
}

// Use LCD_SET_DATA() first.
void lcd_send_data(unsigned char data)
{
    lcd_write_byte(data);
    delay_us(41);
}

// First bit tells which row.
void lcd_goto(unsigned char loc)
{
    LCD_SET_INSTRUCTION();
    if(loc & 0x80)
        lcd_send_instruction(0x80 | (0x40 + loc));
    else
        lcd_send_instruction(0x80 | loc);
}
