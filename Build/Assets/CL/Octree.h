
#define TOPFRONTRIGHT 0
#define TOPFRONTLEFT 1
#define TOPBACKRIGHT 2
#define TOPBACKLEFT 3
#define BOTTOMFRONTRIGHT 4
#define BOTTOMFRONTLEFT 5
#define BOTTOMBACKRIGHT 6
#define BOTTOMBACKLEFT 7

typedef struct
{
	unsigned int data;
	unsigned int color;
} Block;

inline bool getChildPtr(__global Block* b)
{
	return b->data & 65535;
}

inline bool getValid(__global Block* b, unsigned int position)
{
	return b->data & (1 << (16 + position));
}

inline bool getLeaf(__global Block* b, unsigned int position)
{
	return b->data & (1 << (24 + position));
}
