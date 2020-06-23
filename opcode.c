#include <stdio.h>
#include <string.h>
#include "bitops.h"
#include "opcode.h"
#include "ast2600.h"

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
	fwrite(data, sizeof(uint32_t), length, fp);
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

/**
 * extended commands
*/
#define N_LABEL				32
#define N_LABEL_NAME		32
#define N_CODE				512
#define SB_HDR_SIZE_BYTE	0x20
#define SB_HDR_SIZE_DW		(SB_HDR_SIZE_BYTE / sizeof(uint32_t))
#define CM3_BIN_SIZE_DW		(47 * 1024 / sizeof(uint32_t))

typedef struct label_s {
    fpos_t position;
    char name[N_LABEL_NAME];
} label_t;

typedef struct rom_lables_s {
    label_t labels[N_LABEL];
    int count;
} rom_labels_t;

typedef struct log_jmp_s {
    label_t jmps[N_LABEL];
    int count;
} log_jmp_t;
/* log "jump" instructions */

/**
 * global data
*/
rom_labels_t rom_labels = { .count = 0 };

log_jmp_t jmp_list = { .count = 0 };

void log_label(FILE *fp, char *name)
{
    label_t *lab = &rom_labels.labels[rom_labels.count++];

    strcpy(&lab->name[0], name);
    fgetpos(fp, &lab->position);
}

void log_jeq(FILE *fp, uint32_t addr, uint32_t mask, uint32_t target,
		  char *label_name)
{
	label_t *jmp = &jmp_list.jmps[jmp_list.count++];

    fgetpos(fp, &jmp->position);
    strcpy(&jmp->name[0], label_name);

	printf("[%08llx] jeq to %s\n", jmp->position, jmp->name);

    jeq_code(fp, addr, mask, target, 0);
}

void log_jne(FILE *fp, uint32_t addr, uint32_t mask, uint32_t target,
		  char *label_name)
{
    label_t *jmp = &jmp_list.jmps[jmp_list.count++];

    fgetpos(fp, &jmp->position);
    strcpy(&jmp->name[0], label_name);

	printf("[%08llx] jne to %s\n", jmp->position, jmp->name);

    jne_code(fp, addr, mask, target, 0);
}

void log_jmp(FILE *fp, char *label_name)
{
    label_t *jmp = &jmp_list.jmps[jmp_list.count++];

    fgetpos(fp, &jmp->position);
    strcpy(&jmp->name[0], label_name);

	printf("[%08llx] jmp to %s\n", jmp->position, jmp->name);

	/* trick: assert CHIP_ID != 0 */
    jne_code(fp, SCU_BASE + 0x4, 0, 0xffffffff, 0);
}

void print_labels(void)
{
	int i;

	printf("---------------------------------------------------------------\n");
	printf("list of labels\n");
	printf("---------------------------------------------------------------\n");
	for (i = 0; i < rom_labels.count; i++) {
		label_t *lab = &rom_labels.labels[i];
		printf("[%08llx](%8llu) %s\n", (long long)lab->position, (long long)lab->position, lab->name);
	}
}

/**
 *	+------------------+
 *	| start            |
 *	+------------------+
 *	| ...              |
 *	+------------------+
 *	| jump inst. start | -> jmp_list.jmps[i].position
 *	+------------------+
 *	| ...              |
 *	+------------------+
 *	| jump inst. end   |
 *	+------------------+
 *	| next inst.       | -> jmp_list.jmps[i].position + sizeof(rom_op_jmp_t)
 *	+------------------+
 *	| ...              |
 *	+------------------+
 *	| label to jump    | -> rom_labels.labels[j].position
 *	+------------------+
 */
void link_labels(FILE *fp)
{
	int i, j;
	for (i = 0; i < jmp_list.count; i++) {
		for (j = 0; j < rom_labels.count; j++) {
		    if (0 ==
			strcmp(jmp_list.jmps[i].name, rom_labels.labels[j].name)) {
				rom_op_jmp_t code;
				
				fsetpos(fp, &jmp_list.jmps[i].position);
				fread(&code, 1, sizeof(code), fp);
				
				code.cmd.b.num = rom_labels.labels[j].position - (jmp_list.jmps[i].position + sizeof(rom_op_jmp_t));
				
				//printf("code: %08x\n", code.cmd.w);
				fsetpos(fp, &jmp_list.jmps[i].position);
				fwrite(&code, 1, sizeof(code), fp);
		    }
		}
    }

}

void print_rom_patch(FILE *fp)
{
	uint32_t data, size = 0;
    printf("---------------------------------------------------------------\n");
    printf("rom code patch\n");
    printf("---------------------------------------------------------------\n");
	
	fseek(fp, 0, SEEK_SET);
	while (fread(&data, sizeof(uint32_t), 1, fp)) {
		printf("[%08x] %08x\n", size, data);
		size += 4;
	}

	printf("total size: %d bytes\n", size);
}