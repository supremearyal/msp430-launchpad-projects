TARGET = lcddemo
MCU = msp430g2452
FLASHER_DRIVER = rf2500

SRC = $(wildcard *.c)

TOOLCHAIN = msp430
CC = $(TOOLCHAIN)-gcc
CP = $(TOOLCHAIN)-objcopy
OD = $(TOOLCHAIN)-objdump
DBG = $(TOOLCHAIN)-gdb
SIZE = $(TOOLCHAIN)-size

CFLAGS += -Wall
CFLAGS += -Os
CFLAGS += -g
CFLAGS += -mmcu=$(MCU)
# Needed because of bug in gdb
# http://sourceforge.net/p/mspgcc/bugs/332/
CFLAGS += -fomit-frame-pointer

CPFLAGS = -O binary
ODFLAGS = -S

FLASHER = mspdebug

OBJS = $(SRC:.c=.o)

all: $(OBJS) $(TARGET).elf $(TARGET).lst

%.o: %.c
	@echo
	@echo Compiling $<...
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS)
	@echo
	@echo Linking...
	$(CC) $(CFLAGS) -o $(TARGET).elf $(OBJS)

$(TARGET).lst: $(TARGET).elf
	@echo
	@echo Copying...
	$(OD) $(ODFLAGS) $(TARGET).elf > $(TARGET).lst

install: $(TARGET).elf
	@echo
	@echo Flashing...
	$(FLASHER) $(FLASHER_DRIVER) 'prog $(TARGET).elf'

debug: $(TARGET).elf
	@echo
	@echo Debugging...
	$(DBG) $(TARGET).elf

size: $(TARGET).elf
	@echo
	@echo Printing size...
	$(SIZE) $(TARGET).elf

clean:
	@echo
	@echo Cleaning...
	rm -f $(TARGET).elf $(TARGET).lst *.o
