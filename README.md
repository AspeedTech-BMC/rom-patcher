# Aspeed ROM-patch image generator
This tool generates the boot image for Aspeed AST2605 SOC.


## Usage
### compile
```
cd rom-patcher
make
```
### execution
```
./rom-patcher path-to-the-ssp-image
```
#### args
- `path-to-the-ssp-image`: optional.  This argument indicates the path to the ssp image that you want to pack into
the boot image.  If it is not set, `rom-patcher` uses the default ssp image `./zephyr.bin`

## Customization
### Address offset of the patch code image on the firmware SPI Flash
In `config.h`, modify `CONFIG_OFFSET_PATCH_START` to change the offset of the patch code on the firmware SPI Flash memory. The default value is 0x50.

### DRAM configuration
In `config.h`, modify the following options if necessary.
- `#define CONFIG_FPGA_ASPEED`: enable FPGA DRAM configuration (default off)
- `#define CONFIG_ASPEED_DDR4_DUALX8`: enable DDR4 dual X8 die (default off)
- DDR speed
  - `CONFIG_ASPEED_DDR4_1600`: DDR4-1600 AC timing (default speed)
  - `CONFIG_ASPEED_DDR4_800`: DDR4-800 AC timing
