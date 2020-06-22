#include <stdio.h>
#include <string.h>
#include "bitops.h"
#include "opcode.h"

void start_code(FILE *fp)
{
	uint32_t code = START_CODE;
	fwrite(&code, 1, sizeof(code), fp);
}

void quit_code(FILE *fp)
{
	rom_op_quit_t code = {.cmd.b.cmd = 0xf};

	fwrite(&code, 1, sizeof(code), fp);
}

void wr_code(FILE *fp, uint32_t addr, uint32_t length, uint32_t *data)
{
	rom_op_wr_t code = { .cmd.b.cmd = 0x1, .cmd.b.num = length - 1, .addr = addr};

	fwrite(&code, 1, sizeof(code), fp);
	fwrite(data, 1, length, fp);
}

void wr_single(FILE *fp, uint32_t addr, uint32_t value)
{
	uint32_t data = value;

	wr_code(fp, addr, 1, &data);
}

/**
 * delay: micro-second
*/
void waiteq_code(FILE *fp, uint32_t addr, uint32_t mask, uint32_t target, uint32_t delay)
{
	uint32_t tick = delay * TICKS_PER_US;
	rom_op_waiteq_t code = {.cmd.b.cmd = 0x8, .cmd.b.num = tick - 1};
	
	code.addr = addr;
	code.data = target;
	code.mask = mask;
	fwrite(&code, 1, sizeof(code), fp);
}

void waitne_code(FILE *fp, uint32_t addr, uint32_t mask, uint32_t target, uint32_t delay)
{
	uint32_t tick = delay * TICKS_PER_US;
	rom_op_waitne_t code = {.cmd.b.cmd = 0x9, .cmd.b.num = tick - 1};
	
	code.addr = addr;
	code.data = target;
	code.mask = mask;
	fwrite(&code, 1, sizeof(code), fp);
}

void jeq_code(FILE *fp, uint32_t addr, uint32_t mask, uint32_t target, int32_t offset)
{
	rom_op_jmp_t code = {.cmd.b.cmd = 0xc};

	code.cmd.b.num = (uint32_t)offset;
	code.addr = addr;
	code.mask = mask;
	code.data = target;
	fwrite(&code, 1, sizeof(code), fp);
}

void jne_code(FILE *fp, uint32_t addr, uint32_t mask, uint32_t target, int32_t offset)
{
	rom_op_jmp_t code = {.cmd.b.cmd = 0xd};

	code.cmd.b.num = (uint32_t)offset;
	code.addr = addr;
	code.mask = mask;
	code.data = target;
	fwrite(&code, 1, sizeof(code), fp);
}

void delay_code(FILE *fp, uint32_t delay)
{
	rom_op_delay_t code = {.cmd.b.cmd = 0x4};

	code.cmd.b.num = delay * TICKS_PER_US;
	fwrite(&code, 1, sizeof(code), fp);
}

/**
 * *addr = (*addr & mask) + value 
*/
void rmw_code(FILE *fp, uint32_t addr, uint32_t mask, uint32_t value)
{
	rom_op_rmw_t code = {.cmd.b.cmd = 0x2};

	code.addr = addr;
	code.mask = mask;
	code.data = value;
	fwrite(&code, 1, sizeof(code), fp);
}

/**
 * *addr = (*addr & ~value) + 0 
*/
void clrbit_code(FILE *fp, uint32_t addr, uint32_t value)
{
	return rmw_code(fp, addr, ~(value), 0);
}

/**
 * *addr = (*addr & ~value) + value 
*/
void setbit_code(FILE *fp, uint32_t addr, uint32_t value)
{
	return rmw_code(fp, addr, ~(value), value);
}

/**
 * *addr = (*addr & 0xffffffff) + value 
*/
void add_code(FILE *fp, uint32_t addr, uint32_t value)
{
	return rmw_code(fp, addr, GENMASK(31, 0), value);
}

void cp_code(FILE *fp, uint32_t src, uint32_t dst, uint32_t size_dw)
{
	rom_op_cp_t code = {.cmd.b.cmd = 0x3, .cmd.b.num = size_dw};
	code.src = src;
	code.dst = dst;
	fwrite(&code, 1, sizeof(code), fp);
}