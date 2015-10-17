CC = msp430-gcc
CFLAGS = -Wall -Os -mmcu=msp430g2231
SRC = lcdtemp

compile $(SRC).elf: $(SRC).c
	$(CC) $(CFLAGS) $(SRC).c -o $(SRC).elf

assemble $(SRC).s: $(SRC).c
	$(CC) $(CFLAGS) -S $(SRC).c

listing $(SRC).lst: $(SRC).elf
	msp430-objdump -D $(SRC).elf > $(SRC).lst

size: $(SRC).elf
	msp430-size $(SRC).elf

program: $(SRC).elf
	mspdebug rf2500 'prog $(SRC).elf'

clean:
	rm -f $(SRC).elf $(SRC).s $(SRC).lst
