#ifndef DELAY_H_
#define DELAY_H_

// Assumes n > 14.
// Assumes a 1 Mhz clock.
// Can be 3 cycles too long.
void delay_us(register unsigned int n);

// Assumes n > 0.
// Assumes a 1 Mhz clock.
// Not exactly n ms because 2 cycles needed to load %[n], 5 cycles to
// call function and 3 cycles to return from function.
void delay_ms(register unsigned int n);

#endif
