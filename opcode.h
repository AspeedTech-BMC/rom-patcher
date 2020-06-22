#include <stdint.h>
#include <stdio.h>

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

void start_code(FILE *fp);
void quit_code(FILE *fp);
void wr_code(FILE *fp, uint32_t addr, uint32_t length, uint32_t *data);
void wr_single(FILE *fp, uint32_t addr, uint32_t value);
void waiteq_code(FILE *fp, uint32_t addr, uint32_t mask, uint32_t target, uint32_t delay);
void waitne_code(FILE *fp, uint32_t addr, uint32_t mask, uint32_t target, uint32_t delay);
void jeq_code(FILE *fp, uint32_t addr, uint32_t mask, uint32_t target, int32_t offset);
void jne_code(FILE *fp, uint32_t addr, uint32_t mask, uint32_t target, int32_t offset);
void delay_code(FILE *fp, uint32_t delay);
void rmw_code(FILE *fp, uint32_t addr, uint32_t mask, uint32_t value);
void clrbit_code(FILE *fp, uint32_t addr, uint32_t value);
void setbit_code(FILE *fp, uint32_t addr, uint32_t value);
void add_code(FILE *fp, uint32_t addr, uint32_t value);
void cp_code(FILE *fp, uint32_t src, uint32_t dst, uint32_t size_dw);

void log_label(FILE *fp, char *name);
void log_jeq(FILE *fp, uint32_t addr, uint32_t mask, uint32_t target,
		  char *label_name);
void log_jne(FILE *fp, uint32_t addr, uint32_t mask, uint32_t target,
		  char *label_name);
void log_jmp(FILE *fp, char *label_name);

void print_labels(void);
void link_labels(FILE *fp);
void print_rom_patch(FILE *fp);