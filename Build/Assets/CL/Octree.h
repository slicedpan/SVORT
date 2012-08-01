
#define LEFTBOTTOMBACK 0
#define RIGHTBOTTOMBACK 1
#define LEFTTOPBACK 2
#define RIGHTTOPBACK 3
#define LEFTBOTTOMFRONT 4
#define RIGHTBOTTOMFRONT 5
#define LEFTTOPFRONT 6
#define RIGHTTOPFRONT 7

typedef struct
{
	uint numLevels;	
	uint levelOffset[15];
	uint levelSize[16];	//should be enough for a 240TB voxel data size :-)
} OctreeInfo;

typedef struct
{
	uint blockPos;
	uint octantMask;
} BlockInfo;

typedef struct
{
	uint count;
	uint blockInfo[16];
}	VoxelStack;

inline void initStack(VoxelStack* vs)
{
	vs->count = 0;
}

inline void pushVoxel(VoxelStack* vs, uint blockPos, uint octantMask)
{
	vs->blockInfo[vs->count] = blockPos;	//if this overflows 2^29 we're in trouble, however that corresponds to an svo 512GB in size (assuming a block is 8 bytes)
	octantMask = octantMask << 29;
	vs->blockInfo[vs->count] |= octantMask;
	++vs->count;
}

inline BlockInfo popVoxel(VoxelStack* vs)
{
	BlockInfo bi;
	--vs->count;
	bi.blockPos = vs->blockInfo[vs->count] & 536870911;		//if only there were a portable way to define binary literals, this is the first (least significant) 29 bits set
	bi.octantMask = (vs->blockInfo[vs->count] & 3758096384) >> 29;	//last (most significant) three bits set
	return bi;
}

inline bool isEmpty(VoxelStack* vs)
{
	return (vs->count == 0);
}

inline void reduceOctant(uint3* position, uint3* dimensions)
{
	(*dimensions).s0 /= 2;
	(*dimensions).s1 /= 2;
	(*dimensions).s2 /= 2;

	(*position).s0 %= (*dimensions).s0;
	(*position).s1 %= (*dimensions).s1;
	(*position).s2 %= (*dimensions).s2;	
}

inline uint getOctant(uint3 position, uint3 dimensions)
{
	uint octant = RIGHTTOPBACK;	
	dimensions.s0 /= 2;
	dimensions.s1 /= 2;
	dimensions.s2 /= 2;

	if (position.s0 >= dimensions.s0)	//right
	{
		if (position.s1 >= dimensions.s1)	//top
		{
			if (position.s2 >= dimensions.s2)	//front
				octant = RIGHTTOPFRONT;										
		}
		else	//bottom
		{
			if (position.s2 >= dimensions.s2)	//front
				octant = RIGHTBOTTOMFRONT;
			else	//back
				octant = RIGHTBOTTOMBACK;
		}				
	}
	else
	{			
		if (position.s1 >= dimensions.s1)	//top
		{
			if (position.s2 >= dimensions.s2)	//front
				octant = LEFTTOPFRONT;										
			else	//back
				octant = LEFTTOPBACK;
		}
		else	//bottom
		{
			if (position.s2 >= dimensions.s2)	//front
				octant = LEFTBOTTOMFRONT;
			else	//back
				octant = LEFTBOTTOMBACK;
		}			
	}
	return octant;
}

typedef struct
{
	uint data;
	uint colour;
} Block;

inline uint validFlagValue(uint position)
{
	return 1 << (24 + position);
}

inline uint leafFlagValue(uint position)
{
	return 1 << (24 + position);
}

inline uint getChildPtr(__global Block* b)
{
	return b->data & 16777215;	//first 24 bits set
}

inline void setChildPtr(__global Block* b, uint value)
{
	b->data &= ~16777215;		
	b->data |= value & 16777215;
}

inline bool getValid(__global Block* b, unsigned int position)
{
	return !(b->data & (1 << (24 + position)));
}

inline void setValid(__global Block* b, unsigned int position)
{
	b->data |= 1 << (24 + position);
}

inline bool getLeaf(__global Block* b, unsigned int position)
{
	return b->colour & (1 << (24 + position));
}

inline void setLeaf(__global Block* b, unsigned int position)
{
	b->colour |= 1 << (24 + position);
}
