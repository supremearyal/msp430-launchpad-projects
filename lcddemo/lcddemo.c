#include <msp430.h>
#include <intrinsics.h>

#include "display.h"
#include "delay.h"

#define debug() P1DIR |= 1; do { P1OUT ^= 1; delay_ms(500); } while(1)
#define eint() __eint()
#define dint() __dint()
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

volatile unsigned char button_pressed = 0;
volatile unsigned char button_press_pending = 0;
volatile unsigned char gravity_pending = 0;
volatile unsigned char player_row_prev = 32;
volatile unsigned char player_row = 32;
volatile signed char * first_block_col = 0;

void init_cpu(void);
volatile unsigned int rand = 0xFADE;
unsigned int rand_int();

typedef struct
{
    signed char col;
    signed char len;
} block;

static void __attribute__ ((__interrupt__(TIMER0_A0_VECTOR))) gravity(void)
{
    TACTL &= ~MC0;
    //if(player_row == 3 && *first_block_col > 5)
    if(player_row < 32 && *first_block_col > 5)
        player_row_prev = player_row++;
}

static void __attribute__ ((__interrupt__(PORT1_VECTOR))) button_press(void)
{
    P1IFG &= ~_(3); // Need manual interrupt clear.
    //if(player_row == 4)
    //if(player_row > 24 && !button_press_pending)
    if(!button_press_pending)
    {
        button_pressed = 1;
        //player_row_prev = player_row--;   // Move player.
        // Gravity kicks in.
        //TAR = 0;
        //TACTL |= MC0;
    }
}

int main(void)
{
    init_cpu();
    display_init();

    // For gravity.
    TACTL |= TASSEL1 | ID0 | ID1;
    TACCTL0 |= CCIE;
    TACCR0 = 62500 / 2;

    // Initialize register to read pin on interrupt.
    P1DIR &= ~_(3); // Read input.
    P1REN |=  _(3); // Enable pull up/down.
    P1OUT |=  _(3); // Choose pullup.
    P1IES |=  _(3); // Interrupt on high-low transition.
    P1IE  |=  _(3); // Enable pin interrupt.
    eint();         // Enable global interrupt.

    block blocks[2];
    blocks[0].col = 20;
    blocks[0].len = 5;
    blocks[1].col = 35;
    blocks[1].len = 5;
    //blocks[2].col = 50;
    //blocks[2].len = 10;

    first_block_col = &blocks[0].col;

    unsigned char i;
    unsigned char j;
    signed char lim_begin;
    signed char lim_end;
    unsigned char num_blocks = sizeof(blocks) / sizeof(block);

    // Gravity steps.
    unsigned char gravity_up[] = {1, 2, 3, 1, 1, 0, 0, 0};
    unsigned char gravity_down[] = {1, 2, 2, 3, 4, 4};
    unsigned char *gravity_up_begin = gravity_up;
    unsigned char *gravity_up_end = gravity_up + sizeof(gravity_up) / sizeof(gravity_up[0]);
    unsigned char *gravity_down_begin = gravity_down;
    unsigned char *gravity_down_end = gravity_down + sizeof(gravity_down) / sizeof(gravity_down[0]);
    unsigned char *gravity_idx = gravity_up_begin;

    // Display bottom bar.
    display_goto(5, 0);
    DISPLAY_SET_DATA();
    for(i = 0; i < 84; ++i)
    {
        display_send_byte(0xff);
    }

    while(1)
    {
        // Critical section.
        dint();

        // Check for game over.
        //if(blocks[0].col == 4 && player_row > 24)
        //goto game_over;

        // Gravity.
        if(gravity_pending)
        {
            if(gravity_idx != gravity_down_end && player_row != 32)
            {
                player_row_prev = player_row;
                player_row += *gravity_idx++;
            }
            else
            {
                gravity_idx = gravity_up_begin;
                gravity_pending = 0;
            }
        }

        // Handle button press.
        if(!gravity_pending && button_pressed)
        {
            if(gravity_idx != gravity_up_end)
            {
                player_row_prev = player_row;
                player_row -= *gravity_idx++;
                button_press_pending = 1;
            }
            else
            {
                button_press_pending = 0;
                button_pressed = 0;
                // Gravity kicks in.
                gravity_pending = 1;
                gravity_idx = gravity_down_begin;
            }
        }

        // Check to see if landed on top of block.
        unsigned char player_on_block = (blocks[0].col <= 5 && player_row == 24);
        if(player_on_block)
        {
            button_pressed = 0;
            button_press_pending = 0;
            gravity_pending = 0;
            gravity_idx = gravity_up_begin;
        }

        // Display all blocks.
        for(j = 0; j < num_blocks; ++j)
        {
            lim_begin = max(0, blocks[j].col);
            lim_end = min(blocks[j].col + blocks[j].len, 84);

            display_goto(4, lim_begin);
            DISPLAY_SET_DATA();

            for(i = lim_begin; i < lim_end; ++i)
            {
                display_send_byte(0xff);
            }
        }

        // Wait to change frame.
        // Can interrupt here.
        eint();
        delay_ms(33);
        dint();

        // Clear whatever was drawn. No frame buffer.
        for(j = 0; j < num_blocks; ++j)
        {
            if(blocks[j].col == -blocks[j].len)
                continue;

            lim_begin = max(0, blocks[j].col);
            lim_end = min(blocks[j].col + blocks[j].len, 84);

            display_goto(4, lim_begin);
            DISPLAY_SET_DATA();

            for(i = lim_begin; i < lim_end; ++i)
            {
                display_send_byte(0x00);
            }

            blocks[j].col--;
        }

        const unsigned char player[] = {0x08, 0xeb, 0x3f, 0xeb, 0x08};
        // Clear previous player.
        //display_goto(player_row_prev, 0);
        unsigned char player_row_prev_bot = (player_row_prev >> 3);
        unsigned char player_row_prev_aligned =
            player_row_prev - (player_row_prev_bot << 3);
        if(player_row_prev_aligned)
            ++player_row_prev_bot;
        unsigned char player_row_prev_top = player_row_prev_bot - 1;
        display_goto(player_row_prev_bot, 0);
        DISPLAY_SET_DATA();
        for(j = 0; j < sizeof(player) / sizeof(player[0]); ++j)
        {
            display_send_byte(0x00);
        }
        if(player_row_prev_aligned)
        {
            display_goto(player_row_prev_top, 0);
            DISPLAY_SET_DATA();
            for(j = 0; j < sizeof(player) / sizeof(player[0]); ++j)
            {
                display_send_byte(0x00);
            }
        }

        // Draw player.
        //display_goto(player_row, 0);
        unsigned char player_row_bot = (player_row >> 3);
        unsigned char player_row_aligned =
            player_row - (player_row_bot << 3);
        if(player_row_aligned)
            ++player_row_bot;
        unsigned char player_row_top = player_row_bot - 1;
        // Draw bottom part.
        display_goto(player_row_bot, 0);
        DISPLAY_SET_DATA();
        for(j = 0; j < sizeof(player) / sizeof(player[0]); ++j)
        {
            display_send_byte(player[j] >> ((player_row_bot << 3) - player_row));
        }
        if(player_row_aligned)
        {
            // Draw top part.
            display_goto(player_row_top, 0);
            DISPLAY_SET_DATA();
            for(j = 0; j < sizeof(player) / sizeof(player[0]); ++j)
            {
                display_send_byte(player[j] << (player_row - (player_row_top << 3)));
            }
        }

        // If the left most block is off the screen, add a new block.
        if(blocks[0].col == -blocks[0].len)
        {
            //if(player_row == 3)
            //player_row_prev = player_row++;
            if(player_row == 24)
            {
                gravity_pending = 1;
                gravity_idx = gravity_down_begin;
            }

            for(j = 0; j < num_blocks - 1; ++j)
            {
                blocks[j].col = blocks[j + 1].col;
                blocks[j].len = blocks[j + 1].len;
            }

            blocks[j].col = 84;
            blocks[j].len = (rand_int() % 11) + 10;
        }

        eint();
    }

game_over:
    debug();
    // Reset.
    //WDTCTL |= WDTHOLD;

    return 0;
}

void init_cpu(void)
{
    // Disable watchdog timer.
    WDTCTL = WDTPW | WDTHOLD;

    // Calibrate main clock to 1Mhz.
    if(CALBC1_1MHZ == 0xff || CALDCO_1MHZ == 0xff)
        while(1); // Trap if calibration values were erased.
    DCOCTL = 0; // Choose lowest DCO clock and MODx values.
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;
}

unsigned int rand_int()
{
//#define get_bit(a, b) (((a) & (1 << (b))) >> (b))
#define get_bit(a, b) ((a & (1 << b)) >> b)
    rand = (get_bit(rand, 1) ^ get_bit(rand, 2) ^ get_bit(rand, 4) ^ get_bit(rand, 15)) | (rand << 1);
    return rand;
#undef get_bit
}
