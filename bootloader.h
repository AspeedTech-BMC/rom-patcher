#ifndef _BOOTLOADER_H_
#define _BOOTLOADER_H_
#include <stdint.h>
#include <stdio.h>
void copy_cm3(FILE *fp, fpos_t start);
void enable_cm3(FILE *fp);
uint32_t get_cm3_bin_size(void);
void attach_cm3_binary(FILE *fp);
#endif /* end of "#ifndef _BOOTLOADER_H_" */