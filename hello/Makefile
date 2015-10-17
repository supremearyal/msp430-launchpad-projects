CC = msp430-gcc
CFLAGS = -Wall -O2 -mmcu=msp430g2231

hello.elf: hello.c
	$(CC) $(CFLAGS) -S hello.c
	$(CC) $(CFLAGS) hello.c -o hello.elf

program: hello.elf
	mspdebug rf2500 'prog hello.elf'

clean:
	rm -f hello.elf
