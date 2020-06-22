#include <stdint.h>

struct sb_header {
	uint32_t key_location;
	uint32_t enc_img_addr;
	uint32_t img_size;
	uint32_t sign_location;
	uint32_t header_rev[2];
	uint32_t patch_location;	/* address of the rom patch */
	uint32_t checksum;
};

struct cm3_image {
	uint32_t addr;				/* byte address */
	uint32_t size_dw;			/* size is in uint32_t */
};