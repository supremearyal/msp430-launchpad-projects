#include "delay.h"

// Assumes n > 14.
// Assumes a 1 Mhz clock.
// Can be 3 cycles too long.
void delay_us(register unsigned int n)
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
        "3: dec %[n]       \n\t"    // Have to use local labels (numbers).
        "   nop            \n\t"    // Nop (single cycle, single byte).
        "   jne 3b           \n"    // Jump backwards.
        : [n] "+r" (n));
}

// Assumes n > 0.
// Assumes a 1 Mhz clock.
// Not exactly n ms because 2 cycles needed to load %[n], 5 cycles to
// call function and 3 cycles to return from function.
void delay_ms(register unsigned int n)
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
