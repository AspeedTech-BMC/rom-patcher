#include <stdio.h>
#include <string.h>
#include "bitops.h"
#include "opcode.h"

uint32_t* start_code(uint32_t *ptr)
{
	*ptr++ = START_CODE;
	return ptr;
}

uint32_t* quit_code(uint32_t *ptr)
{
	rom_op_quit_t code = {.cmd.b.cmd = 0xf};
	*ptr++ = code.cmd.w;
	return ptr;
}

uint32_t* wr_code(uint32_t *ptr, uint32_t addr, uint32_t length, uint32_t *data)
{
	rom_op_wr_t code = { .cmd.b.cmd = 0x1, .cmd.b.num = length - 1, .addr = addr};

	memcpy(ptr, &code, sizeof(rom_op_wr_t));
	ptr += sizeof(rom_op_wr_t) / sizeof(uint32_t);
	memcpy(ptr, data, length * sizeof(uint32_t));
	ptr += length;

	return ptr;
}

uint32_t* wr_single(uint32_t *ptr, uint32_t addr, uint32_t value)
{
	uint32_t data = value;
	return  wr_code(ptr, addr, 1, &data);
}

/**
 * delay: micro-second
*/
uint32_t* waiteq_code(uint32_t *ptr, uint32_t addr, uint32_t mask, uint32_t target, uint32_t delay)
{
	uint32_t tick = delay * TICKS_PER_US;
	rom_op_waiteq_t code = {.cmd.b.cmd = 0x8, .cmd.b.num = tick - 1};
	
	code.addr = addr;
	code.data = target;
	code.mask = mask;
	memcpy(ptr, &code, sizeof(rom_op_waiteq_t));
	ptr += sizeof(rom_op_waiteq_t) / sizeof(uint32_t);
	
	return ptr;
}

uint32_t* waitne_code(uint32_t *ptr, uint32_t addr, uint32_t mask, uint32_t target, uint32_t delay)
{
	uint32_t tick = delay * TICKS_PER_US;
	rom_op_waitne_t code = {.cmd.b.cmd = 0x9, .cmd.b.num = tick - 1};
	
	code.addr = addr;
	code.data = target;
	code.mask = mask;
	memcpy(ptr, &code, sizeof(rom_op_waitne_t));
	ptr += sizeof(rom_op_waitne_t) / sizeof(uint32_t);
	
	return ptr;
}

uint32_t* jeq_code(uint32_t *ptr, uint32_t addr, uint32_t mask, uint32_t target, int32_t offset)
{
	rom_op_jmp_t code = {.cmd.b.cmd = 0xc};

	code.cmd.b.num = (uint32_t)offset;
	code.addr = addr;
	code.mask = mask;
	code.data = target;
	memcpy(ptr, &code, sizeof(rom_op_jmp_t));
	ptr += sizeof(rom_op_jmp_t) / sizeof(uint32_t);

	return ptr;
}

uint32_t* jne_code(uint32_t *ptr, uint32_t addr, uint32_t mask, uint32_t target, int32_t offset)
{
	rom_op_jmp_t code = {.cmd.b.cmd = 0xd};

	code.cmd.b.num = (uint32_t)offset;
	code.addr = addr;
	code.mask = mask;
	code.data = target;
	memcpy(ptr, &code, sizeof(rom_op_jmp_t));
	ptr += sizeof(rom_op_jmp_t) / sizeof(uint32_t);

	return ptr;
}

uint32_t* delay_code(uint32_t *ptr, uint32_t delay)
{
	rom_op_delay_t code = {.cmd.b.cmd = 0x4};

	code.cmd.b.num = delay * TICKS_PER_US;
	memcpy(ptr, &code, sizeof(rom_op_delay_t));
	ptr += sizeof(rom_op_delay_t) / sizeof(uint32_t);

	return ptr;
}

/**
 * *addr = (*addr & mask) + value 
*/
uint32_t* rmw_code(uint32_t *ptr, uint32_t addr, uint32_t mask, uint32_t value)
{
	rom_op_rmw_t code = {.cmd.b.cmd = 0x2};

	code.addr = addr;
	code.mask = mask;
	code.data = value;
	memcpy(ptr, &code, sizeof(rom_op_rmw_t));
	ptr += sizeof(rom_op_rmw_t) / sizeof(uint32_t);

	return ptr;
}

/**
 * *addr = (*addr & ~value) + 0 
*/
uint32_t* clrbit_code(uint32_t *ptr, uint32_t addr, uint32_t value)
{
	return rmw_code(ptr, addr, ~(value), 0);
}

/**
 * *addr = (*addr & ~value) + value 
*/
uint32_t* setbit_code(uint32_t *ptr, uint32_t addr, uint32_t value)
{
	return rmw_code(ptr, addr, ~(value), value);
}

/**
 * *addr = (*addr & 0xffffffff) + value 
*/
uint32_t* add_code(uint32_t *ptr, uint32_t addr, uint32_t value)
{
	return rmw_code(ptr, addr, GENMASK(31, 0), value);
}

uint32_t* cp_code(uint32_t *ptr, uint32_t src, uint32_t dst, uint32_t size_dw)
{
	rom_op_cp_t code = {.cmd.b.cmd = 0x3, .cmd.b.num = size_dw};
	code.src = src;
	code.dst = dst;
	memcpy(ptr, &code, sizeof(rom_op_cp_t));
	ptr += sizeof(rom_op_cp_t) / sizeof(uint32_t);

	return ptr;
}