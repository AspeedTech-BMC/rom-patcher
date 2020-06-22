.PHONY: all clean

all:
	gcc opcode.c ddr4_init.c bootloader.c main.c -o rom_patch


clean:
	@rm -f *.o *.bin rom_patch