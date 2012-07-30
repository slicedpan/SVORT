
#ifdef _WIN32
#pragma once
#define uint cl_uint
#define int3 cl_int3
#define uint3 cl_uint3
#define s0 s[0]
#define s1 s[1]
#define s2 s[2]
#define __global
namespace SVO
{
#endif

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
	return b->data & 16777215;
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

#ifdef _WIN32
}
#endif