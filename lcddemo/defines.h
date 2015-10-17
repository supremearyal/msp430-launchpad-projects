#ifndef DEFINES_H_
#define DEFINES_H_

#define DISPLAY_DIR  P1DIR
#define DISPLAY_OUT  P1OUT

#define _(x)         (1 << x)

#define DISPLAY_RES   _(1)
#define DISPLAY_SCE   _(2)
#define DISPLAY_MODE  _(4)
#define DISPLAY_SCLK  _(5)
#define DISPLAY_SDOUT _(6)
#define DISPLAY_SDIN  _(7)

#endif
