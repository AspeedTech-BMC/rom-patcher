.PHONY: all clean

all:
	gcc opcode.c ddr4_init.c bootloader.c main.c -o rom-patcher


clean:
	@rm -f *.o rom-patcher boot.bin rom_patch.bin