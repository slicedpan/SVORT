
#ifndef _OCTREE_H
#define _OCTREE_H

#include "CLDefsBegin.h"

#define LEFTBOTTOMBACK 0
#define RIGHTBOTTOMBACK 1
#define LEFTTOPBACK 2
#define RIGHTTOPBACK 3
#define LEFTBOTTOMFRONT 4
#define RIGHTBOTTOMFRONT 5
#define LEFTTOPFRONT 6
#define RIGHTTOPFRONT 7

#define XMASK 1
#define YMASK 2
#define ZMASK 4

#define BIPOSMASK 536870911
#define BIOCTMASK 3758096384

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
	ushort4 centre;
} BlockInfo;

typedef struct
{
	uint count;	
	uint blockInfo[15];	
	ushort4 coords[15];
	uint maxLen;
}	VoxelStack;

inline void initStack(VoxelStack* vs, uint maxSideLength)
{
	vs->count = 0;
	uint mid = maxSideLength >> 1;
	vs->coords[0].s0 = mid;
	vs->coords[0].s1 = mid;
	vs->coords[0].s2 = mid;
	vs->maxLen = maxSideLength;
}

inline void pushVoxel(VoxelStack* vs, uint blockPos, uint octantMask)
{
	vs->blockInfo[vs->count] = blockPos;	//if this overflows 2^29 we're in trouble, however that corresponds to an svo 512GB in size (assuming a block is 8 bytes)		

	uint sideLength = vs->maxLen >> (vs->count + 2);
	vs->coords[vs->count + 1].s0 = vs->coords[vs->count].s0 + (((octantMask & XMASK) << 1) - 1) * sideLength;
	vs->coords[vs->count + 1].s1 = vs->coords[vs->count].s1 + ((octantMask & YMASK) - 1) * sideLength;
	vs->coords[vs->count + 1].s2 = vs->coords[vs->count].s2 + (((octantMask & ZMASK) >> 1) - 1) * sideLength;
	octantMask = octantMask << 29;
	vs->blockInfo[vs->count] |= octantMask;
	++vs->count;	
}

inline BlockInfo popVoxel(VoxelStack* vs)
{
	BlockInfo bi;
	--vs->count;
	bi.blockPos = vs->blockInfo[vs->count] & BIPOSMASK;		//first (least significant) 29 bits
	bi.octantMask = (vs->blockInfo[vs->count] & BIOCTMASK) >> 29;	//last (most significant) three bits
	bi.centre = vs->coords[vs->count + 1];
	return bi;
}

inline BlockInfo peekVoxel(VoxelStack* vs, uint index)
{
	BlockInfo bi;
	bi.blockPos = vs->blockInfo[index] & BIPOSMASK;
	bi.octantMask = (vs->blockInfo[index] & BIOCTMASK) >> 29;
	return bi;
}

inline bool isEmpty(VoxelStack* vs)
{
	return (vs->count == 0);
}

#ifdef NONPOWEROFTWOSIZES

inline void reduceOctant(uint3* position, uint3* dimensions)
{
	(*dimensions).s0 /= 2;
	(*dimensions).s1 /= 2;
	(*dimensions).s2 /= 2;

	(*position).s0 %= (*dimensions).s0;
	(*position).s1 %= (*dimensions).s1;
	(*position).s2 %= (*dimensions).s2;	
}

inline uint getAndReduceOctant(uint3* position, uint3* dimensions)
{
	uint octant = RIGHTTOPBACK;	
	(*dimensions).s0 /= 2;
	(*dimensions).s1 /= 2;
	(*dimensions).s2 /= 2;

	if ((*position).s0 >= (*dimensions).s0)	//right
	{
		if ((*position).s1 >= (*dimensions).s1)	//top
		{
			if ((*position).s2 >= (*dimensions).s2)	//front
				octant = RIGHTTOPFRONT;										
		}
		else	//bottom
		{
			if ((*position).s2 >= (*dimensions).s2)	//front
				octant = RIGHTBOTTOMFRONT;
			else	//back
				octant = RIGHTBOTTOMBACK;
		}				
	}
	else
	{			
		if ((*position).s1 >= (*dimensions).s1)	//top
		{
			if ((*position).s2 >= (*dimensions).s2)	//front
				octant = LEFTTOPFRONT;										
			else	//back
				octant = LEFTTOPBACK;
		}
		else	//bottom
		{
			if ((*position).s2 >= (*dimensions).s2)	//front
				octant = LEFTBOTTOMFRONT;
			else	//back
				octant = LEFTBOTTOMBACK;
		}			
	}

	(*position).s0 %= (*dimensions).s0;
	(*position).s1 %= (*dimensions).s1;
	(*position).s2 %= (*dimensions).s2;	

	return octant;
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

#else

inline void reduceOctant(uint4* position, uint4* dimensions)
{
	(*dimensions).s0 >>= 1;
	(*dimensions).s1 >>= 1;
	(*dimensions).s2 >>= 1;

	(*position).s0 %= (*dimensions).s0;
	(*position).s1 %= (*dimensions).s1;
	(*position).s2 %= (*dimensions).s2;	
}

inline uint getOctant(uint4 position, uint4 dimensions)
{
	uint octant = 0;	
	dimensions.s0 >>= 1;
	dimensions.s1 >>= 1;
	dimensions.s2 >>= 1;

	uint v = dimensions.s0 & position.s0;	//sets one bit in v -> Power of two
	uint f = v && !(v & (v - 1));	//sets f to one if v is a power of two (and not zero)

	octant = (octant & ~XMASK) | (-f & XMASK);

	v = dimensions.s1 & position.s1;
	f = v && !(v & (v - 1));

	octant = (octant & ~YMASK) | (-f & YMASK);

	v = dimensions.s2 & position.s2;
	f = v && !(v & (v - 1));

	octant = (octant & ~ZMASK) | (-f & ZMASK);
	
	return octant;
}

inline uint getAndReduceOctant(uint4* position, uint4* dimensions)
{
	(*dimensions).s0 >>= 1;
	(*dimensions).s1 >>= 1;
	(*dimensions).s2 >>= 1;

	uint octant = 0;	

	uint v = (*dimensions).s0 & (*position).s0;
	uint f = v && !(v & (v - 1));	

	octant = (octant & ~XMASK) | (-f & XMASK);

	v = (*dimensions).s1 & (*position).s1;
	f = v && !(v & (v - 1));	

	octant = (octant & ~YMASK) | (-f & YMASK);

	v = (*dimensions).s2 & (*position).s2;
	f = v && !(v & (v - 1));	

	octant = (octant & ~ZMASK) | (-f & ZMASK);

	(*position).s0 %= (*dimensions).s0;
	(*position).s1 %= (*dimensions).s1;
	(*position).s2 %= (*dimensions).s2;	
	
	return octant;
}

#endif



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

#endif