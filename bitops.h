#define BIT(nr)                 (1 << (nr))
#define GENMASK(h, l) \
	(((~0U) << (l)) & (~0U >> (32 - 1 - (h))))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))