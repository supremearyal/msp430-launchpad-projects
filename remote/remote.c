#include <msp430.h>
#include <intrinsics.h>

#define eint()    __eint()
#define dint()    __dint()

#define UART_TX   (1 << 1)
#define IR_SENSOR (1 << 4)

#if 0
// Assumes n > 0.
// Assumes a 8 Mhz clock.
static void delay_us(register unsigned int n)
{
    // Need to do 8 cycles * n.
    __asm__ __volatile__ (
        "1: jmp $+2        \n\t" // 2 cycle nop.
        "   jmp $+2        \n\t" // 2 cycle nop.
        "   nop            \n\t" // 1 cycle nop.
        "   dec %[n]       \n\t" // 1 cycle.
        "   jne 1b           \n" // 2 cycle.
        : [n] "+r" (n));
}

// Assumes n > 0.
// Assumes a 8 Mhz clock.
// Not exactly n ms because 2 cycles needed to load %[n], 5 cycles to
// call function and 3 cycles to return from function.
static void delay_ms(register unsigned int n)
{
    __asm__ __volatile__ (
        "1: mov #2664, r14 \n\t" // 2 + 3 * 2664 + 2 + 1 + 1 + 2 = 8000 cycles.
        "2: dec r14        \n\t" // Have to use local labels (numbers).
        "   jne 2b         \n\t" // Jump backwards.
        "   jmp $+2        \n\t" // Two cycle single byte nop.
        "   nop            \n\t" // Single byte nop.
        "   dec %[n]       \n\t" // Delay n 1ms cycles.
        "   jne 1b           \n" // Jump backwards.
        : [n] "+r" (n));
}
#endif

#define NUM_SAMPLES 201
// First byte is the header.
volatile unsigned char sample[NUM_SAMPLES] = {0x20};
// Already have one byte (header).
volatile unsigned int sample_index = 1;
volatile unsigned char sample_bit_index = 0;
volatile unsigned char bit_to_send = 0;
volatile unsigned int transmit_index = 0;
volatile unsigned char transmit_bit_index = 1;
volatile unsigned char transmit = 0;
volatile unsigned char transmitting = 0;
#define START_BIT 0
#define DATA_BIT 1
#define STOP_BIT 2
volatile unsigned char transmit_state = START_BIT;

// Ir receiver interrupt on high to low transition.
static void __attribute__ ((__interrupt__(PORT1_VECTOR))) start_sample(void)
{
    // Don't interrupt while sampling.
    P1IE &= ~IR_SENSOR;
    // Start timer to capture signal.
    // Sampling rate = 100 kHz.
    // 2x ir transmission of ~ 40 kHz.
    // Nyquist theorem.
    TACCR0 = 80;
}

// Sample pin or transmit.
static void __attribute__ ((__interrupt__(TIMER0_A0_VECTOR))) add_point(void)
{
    if(!transmit)
    {
        if(sample_bit_index == 0)
            sample[sample_index] = 0;

        sample[sample_index] |=
            ((P1IN & IR_SENSOR) >> 4) << sample_bit_index++;

        if(sample_bit_index == 8)
        {
            sample_index++;
            sample_bit_index = 0;
        }

        // Done sampling.
        if(sample_index == NUM_SAMPLES)
        {
            TACCR0 = 0;
            // Can transmit now.
            transmitting = 1;
        }
    }
    else
    {
        switch(transmit_state)
        {
        case START_BIT:
            P1OUT &= ~UART_TX;
            transmit_state = DATA_BIT;
            break;
        case DATA_BIT:
            if(bit_to_send)
                P1OUT |= UART_TX;
            else
                P1OUT &= ~UART_TX;

            if(transmit_bit_index == 8)
            {
                transmit_index++;
                transmit_bit_index = 0;
                transmit_state = STOP_BIT;
            }
            else
            {
                transmit_state = DATA_BIT;
            }

            bit_to_send =
                (sample[transmit_index] >> transmit_bit_index++) & 0x1;
            break;
        case STOP_BIT:
            P1OUT |= UART_TX;
            transmit_state = START_BIT;
            // All bits done.
            if(transmit_index == NUM_SAMPLES)
            {
                TACCR0 = 0;
                // Already have header byte.
                sample_index = 1;
                sample_bit_index = 0;
                // Wait for another sample sequence.
                transmit = 0;
                P1IFG &= ~IR_SENSOR;
                P1IE |= IR_SENSOR;
            }
            break;
        default:
            break;
        }
    }
}

int main(void)
{
    // Disable watchdog timer.
    WDTCTL = WDTPW | WDTHOLD;

    // Calibrate main clock to 8Mhz.
    if(CALBC1_8MHZ == 0xff || CALDCO_8MHZ == 0xff)
        while(1); // Trap if calibration values were erased.
    DCOCTL = 0; // Choose lowest DCO clock and MODx values.
    BCSCTL1 = CALBC1_8MHZ;
    DCOCTL = CALDCO_8MHZ;

    // Pin interrupt used to detect ir signal.

    // Set ir sensor pin as input to detect high to low transition.
    P1DIR &= ~IR_SENSOR;
    // No need to choose pullup. Built in pullup on ir receiver.
    // Ir sensor detected on high to low transition.
    P1IES |= IR_SENSOR;
    // Enable interrupt on ir sensor pin.
    P1IE |= IR_SENSOR;
    // Clear existing interrupts for ir sensor.
    P1IFG &= ~IR_SENSOR;

    // Timer used for sampling and uart.

    // Use cpu clock for timer.
    TACTL |= TASSEL1;
    // Don't divide input clock.
    // Count up.
    TACTL |= MC0;
    // Enable Timer A TACCR0 interrupt.
    TACCTL0 |= CCIE;
    // Don't start timer yet.
    TACCR0 = 0;

    // UART_TX used to send uart data.
    P1DIR |= UART_TX;
    // Uart transmit pin is high by default.
    P1OUT |= UART_TX;

    // Enable global interrupt.
    eint();

    while(1)
    {
        if(transmitting)
        {
            // Start transmitting from first byte and bit.
            transmit_index = 0;
            transmit_bit_index = 1;
            // First bit is always the same.
            bit_to_send = 0;
            // Set to transmit data.
            transmit = 1;
            // Currently transmitting, don't enter this code until
            // another sample read.
            transmitting = 0;
            // 9600 bps at 8 Mhz.
            TACCR0 = 833;
        }
    }

    return 0;
}
