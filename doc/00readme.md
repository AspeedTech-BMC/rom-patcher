# AST2600 ROM patch code generator
## Usage
### step 1: configure `CONFIG_OFFSET_PATCH_START` in `config.h`
this is the address offset of this patch code binary that will be placed on the SPI Flash.
The default value is 0x50.  Modify it if necessary, then execute `make` to generate `rom_patch`

Note: this step can be ignored if `CONFIG_OFFSET_PATCH_START` isn't changed.

### step 2: update CM3 binary
update `ast2600_ssp.bin` to the root folder

Note: this step can be ignored if `ast2600_ssp.bin` isn't changed.

### step 3: execution
execute `./rom_patch`, the output file `rom_patch.bin` and `boot.bin` will be generated.
- `rom_patch.bin`: only ROM patch code and CM3 image
- `boot.bin`: including CA7 jump code, secure boot header, ROM patch and CM3 image

## Generate patch code for different targets

Modify `config.h` and re-compile `rom_patch`

- `CONFIG_FPGA_ASPEED`: for FPGA
- `CONFIG_ASPEED_DDR4_DUALX8`: DDR4 dual X8 die
- AC timing
  - `CONFIG_ASPEED_DDR4_1600`: DDR4-1600 AC timing
  - `CONFIG_ASPEED_DDR4_800`: DDR4-800 AC timing