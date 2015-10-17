#include <msp430.h>

#define RED_LED   (1 << 0)
#define GREEN_LED (1 << 6)

// Assumes n > 6.
// Assumes a 1 Mhz clock.
// Won't be exactly 1us.
static void __inline__ delay_us(register unsigned int n)
{
	__asm__ __volatile__ (
		"    sub #6, %[n] \n\t"         // 4 * n + 6 cycles means n us.
		"    rra %[n]     \n\t"
		"    rra %[n]     \n\t"
		"1:  dec %[n]     \n\t"         // Have to use local labels (numbers).
		"    nop          \n\t"         // Nop (single cycle, single byte).
		"    jne 1b         \n"         // Jump backwards.
		: [n] "+r" (n));
}

// Assumes n > 0.
// Assumes a 1 Mhz clock.
// Exactly n ms.
static void __inline__ delay_ms(register unsigned int n)
{
	__asm__ __volatile__ (
		"1:  mov #331, r14  \n\t"         // 2 + 3 * 331 + 1 + 2 + 2 =
		                                  // 1000 cycles
		"2:  dec r14        \n\t"         // Have to use local labels (numbers).
		"    jne 2b         \n\t"         // Jump backwards.
		"    jmp $+2        \n\t"         // Two cycle single byte nop.
		"    dec %[n]       \n\t"         // Delay n 1ms cycles.
		"    jne 1b           \n"         // Jump backwards.
		: [n] "+r" (n));
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

	// Set port direction.
	P1DIR |= (RED_LED | GREEN_LED);

	while(1)
	{
		// Delay for 500 ms.
		delay_ms(500);
		// Toggle pin.
		P1OUT ^= (RED_LED | GREEN_LED);
	}

	return 0;
}
