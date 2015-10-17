#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "defines.h"

#define DISPLAY_SET_DATA() DISPLAY_OUT |= DISPLAY_MODE
#define DISPLAY_SET_CMD() DISPLAY_OUT &= ~DISPLAY_MODE

#define DISPLAY_GOTO_COL(col) display_send_byte(0x80 | (col))
#define DISPLAY_GOTO_ROW(row) display_send_byte(0x40 | (row))

#define DISPLAY_START_TRANSMIT() DISPLAY_OUT &= ~DISPLAY_SCE
#define DISPLAY_END_TRANSMIT() DISPLAY_OUT |= DISPLAY_SCE

void display_init(void);
void display_goto(unsigned char row, unsigned char col);
void display_clear(void);
void display_send_byte(unsigned char byte);

#endif
