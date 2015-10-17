#include <msp430.h>

#define LCD_DIR P1DIR
#define LCD_OUT P1OUT
#define LCD_RS   (1 << 5)
#define LCD_E    (1 << 4)

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
        "1: mov #331, r14  \n\t"   // 2 + 3 * 331 + 1 + 2 + 2 = 1000 cycles.
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
void lcd_set_fonts(void);
void lcd_goto(unsigned char loc);
void lcd_disp_digit(unsigned char digit);

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

    LCD_DIR |= 0xf;
    LCD_DIR |= (LCD_RS | LCD_E);

    lcd_initialize();
    lcd_set_fonts();
    
#define REPEAT_SINGLE_CHANNEL CONSEQ1
#define TEMPERATURE_SENSOR (INCH1 | INCH3)
#define INTERNAL_REFERENCE_AND_GND SREF0

    // Initialize ADC10 stuff.
    ADC10CTL0 |= ADC10ON; // Turn on A2D.
    ADC10CTL1 |= REPEAT_SINGLE_CHANNEL; // Repeat single channel.
    // Sample hold source is ADC10SC. No need to set bits.
    ADC10CTL1 |= TEMPERATURE_SENSOR; // Choose built in temperature sensor.
    ADC10CTL0 |= INTERNAL_REFERENCE_AND_GND; // Use internal reference for V_R+ and V_SS for V_R-.
    ADC10CTL0 |= REFON; // Enable internal reference.
    delay_ms(1); // Wait for reference to settle.
    // Use 1.5 V reference by default.
    ADC10CTL0 |= ADC10SHT0 | ADC10SHT1; // 64 clocks.
    ADC10CTL0 |= ENC | ADC10SC; // Enable conversion and start conversion.

    static unsigned int temps[8]; // Holds 8 most recent temperatures
                                  // (circular buffer).
    unsigned char i; // Index to circular buffer.
    unsigned char j; // Index to compute average.

    // Wait for conversion to finish.
    while(!(ADC10CTL0 & ADC10IFG))
        ;
    ADC10CTL0 &= ~ADC10IFG;

    // First time around fill all 8 samples with sample temperature to
    // make average formula same with only 1 sample.
    for(i = 0; i < 8; ++i)
        temps[i] = ADC10MEM;

    while(1)
    {
        // Get average temperature.
        unsigned int n = 0;
        for(j = 0; j < 8; ++j)
            n += temps[j];
        n >>= 3;

        // Convert to fahrenheit.
        n = (761 * (n - 630L)) >> 10;

        // Display temperature.
        lcd_disp_digit((0x0 << 4) | (n / 10));
        lcd_disp_digit((0x4 << 4) | (n % 10));
        lcd_goto(0x8); LCD_SET_DATA(); lcd_send_data(0xdf);
        lcd_send_data('F');

        // Need to disable so as to start conversion later.
        // See salu144i page 556.
        ADC10CTL0 &= ~ENC;

        // Start conversion again.
        ADC10CTL0 |= ENC | ADC10SC;

        // Wait for conversion to finish.
        while(!(ADC10CTL0 & ADC10IFG))
            ;
        ADC10CTL0 &= ~ADC10IFG;

        // Circular buffer.
        if(i == 8)
            i = 0;

        // Add to recent samples.
        temps[i++] = ADC10MEM;

        // Wait a little bit so display isn't erratic.
        delay_ms(300);
    }
    
    return 0;
}

// Initialization sequence for 4-bit access from HD44780 datasheet.
void lcd_initialize(void)
{
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

// Put fonts into CGRAM.
// Crazy switch statement.
void lcd_set_fonts(void)
{
    unsigned char count = 0;
    unsigned char i;
    unsigned char fill = 0x1f;
    // Only need five font characters.
    while(count < 5)
    {
        LCD_SET_INSTRUCTION();
        lcd_send_instruction(0x40 | (count << 3));
        LCD_SET_DATA();
        i = 0;
        switch(count++)
        {
        case 0:
        case 4:
            // Bar: all ones.
            // Space: all blank.
            if(count == 5)
                fill = 0x00;
            while(i++ < 8)
                lcd_send_data(fill);
            break;
        case 1:
            lcd_send_data(0x1f);
            lcd_send_data(0x1f);
        case 2:
            while(i++ < 6)
                lcd_send_data(0x00);
            if(count == 2)
                break;
        case 3:
            lcd_send_data(0x1f);
            lcd_send_data(0x1f);
            if(count == 3)
                break;
            while(i++ < 4)
                lcd_send_data(0x00);
            lcd_send_data(0x1f);
            lcd_send_data(0x1f);
            break;
        default:
            break;
        }
    }
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

// Encoded map for each digit.
// Each cell is three bits to represent 5 possible characters.
// Each byte contains two cells.
// Three bytes for each digit equals 6 cells for each digit.
// The codes are 0b000 to 0b100 from lcd_set_fonts.
const unsigned char digit_map[] = {0x08, 0x00, 0x02,
                                   0x01, 0x14, 0x10,
                                   0x1b, 0x00, 0x12,
                                   0x19, 0x10, 0x02,
                                   0x10, 0x20, 0x04,
                                   0x18, 0x13, 0x02,
                                   0x18, 0x03, 0x02,
                                   0x09, 0x20, 0x04,
                                   0x18, 0x00, 0x02,
                                   0x18, 0x10, 0x02};

// First four bits are location 0 .. 15.
// Last four bits are the digit 0 .. 10.
void lcd_disp_digit(unsigned char digit)
{
    // Extract row.
    unsigned char location = (digit >> 4);
    // Top bits are unimportant as far as the digit is concerned.
    digit &= 0x0f;
    lcd_goto(location);

    LCD_SET_DATA();
    unsigned char i = 0;
    while(i < 3)
    {
        // Get pattern for this digit.
        // i + (digit << 1) + digit == i * 3
        unsigned char pattern = digit_map[i + (digit << 1) + digit];
        // Extract first encoded cell for this byte.
        lcd_send_data(pattern & 0x7);
        // After third cell, need to go to the bottom row.
        if(i++ == 1)
        {
            lcd_goto(location | 0x80);
            LCD_SET_DATA();
        }
        // Extract second encoded cell for this byte.
        lcd_send_data(pattern >> 3);
    }
}
