# AST2600 ROM patch code generator
## usage
### step 1: configure `CONFIG_OFFSET_PATCH_START` in `config.h`
this is the address offset of this patch code binary that will be placed on the SPI Flash.
Then, execute `make` to generate `rom_patch`

Note: this step can be ignored if `CONFIG_OFFSET_PATCH_START` isn't changed.

### step 2: update CM3 binary
update `ast2600_ssp.bin` to the root folder

Note: this step can be ignored if `ast2600_ssp.bin` isn't changed.

### step 3: execution
execute `./rom_patch`, the output file `rom_patch.bin` will be generated.