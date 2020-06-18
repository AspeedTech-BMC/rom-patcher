#include <stdint.h>
#define START_CODE	0x1eadc0de
#define TICKS_PER_US	(1000 / 40)

typedef union rom_op_cmd_s {
	struct {
		uint32_t cmd : 8;
		uint32_t num : 24;
	} b;
	uint32_t w;
} rom_op_cmd_t;

typedef struct rom_op_wr_s {
	rom_op_cmd_t cmd;
	uint32_t addr;
} rom_op_wr_t;

typedef struct rom_op_waiteq_s {
	rom_op_cmd_t cmd;
	uint32_t addr;
	uint32_t data;
	uint32_t mask;
} rom_op_waiteq_t;

typedef struct rom_op_waitne_s {
	rom_op_cmd_t cmd;
	uint32_t addr;
	uint32_t data;
	uint32_t mask;
} rom_op_waitne_t;

typedef struct rom_op_jmp_s {
	rom_op_cmd_t cmd;
	uint32_t addr;
	uint32_t data;
	uint32_t mask;
} rom_op_jmp_t;

typedef struct rom_op_delay_s {
	rom_op_cmd_t cmd;
} rom_op_delay_t;

typedef struct rom_op_rmw_s {
	rom_op_cmd_t cmd;
	uint32_t addr;
	uint32_t data;
	uint32_t mask;
} rom_op_rmw_t;

typedef struct rom_op_cp_s {
	rom_op_cmd_t cmd;
	uint32_t src;
	uint32_t dst;
} rom_op_cp_t;

typedef struct rom_op_quit_s {
	rom_op_cmd_t cmd;
} rom_op_quit_t;

uint32_t* start_code(uint32_t *ptr);
uint32_t* quit_code(uint32_t *ptr);
uint32_t* wr_code(uint32_t *ptr, uint32_t addr, uint32_t length, uint32_t *data);
uint32_t* wr_single(uint32_t *ptr, uint32_t addr, uint32_t value);
uint32_t* waiteq_code(uint32_t *ptr, uint32_t addr, uint32_t mask, uint32_t target, uint32_t delay);
uint32_t* waitne_code(uint32_t *ptr, uint32_t addr, uint32_t mask, uint32_t target, uint32_t delay);
uint32_t* jeq_code(uint32_t *ptr, uint32_t addr, uint32_t mask, uint32_t target, int32_t offset);
uint32_t* jne_code(uint32_t *ptr, uint32_t addr, uint32_t mask, uint32_t target, int32_t offset);
uint32_t* delay_code(uint32_t *ptr, uint32_t delay);
uint32_t* rmw_code(uint32_t *ptr, uint32_t addr, uint32_t mask, uint32_t value);
uint32_t* clrbit_code(uint32_t *ptr, uint32_t addr, uint32_t value);
uint32_t* setbit_code(uint32_t *ptr, uint32_t addr, uint32_t value);
uint32_t* add_code(uint32_t *ptr, uint32_t addr, uint32_t value);
uint32_t* cp_code(uint32_t *ptr, uint32_t src, uint32_t dst, uint32_t size_dw);
