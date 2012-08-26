
#ifndef _OCTREE_H
#define _OCTREE_H

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
#define ALLMASKS 7

#ifndef HOSTINCLUDE
#define OCTANTMASKS (int4)(1, 2, 4, 0)
#define FZEROS (float4)(0.0f, 0.0f, 0.0f, 0.0f)
#define FONES (float4)(1.0f, 1.0f, 1.0f, 1.0f)
#endif

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

	uint xSwitch;
	uint ySwitch;
	uint zSwitch;

	uint junk1;
	uint blockPos[15];	
	uint junk2;
	uint octant[15];

}	VoxelStack;

inline void initStack(VoxelStack* vs, uint maxSideLength)
{
	vs->count = 0;

#ifdef STACKSWITCH
	vs->xSwitch = 1;
	vs->ySwitch = 1;
	vs->zSwitch = 1;
#endif

}

inline void pushVoxel(VoxelStack* vs, uint blockPos, uint octantMask)
{
	vs->blockPos[vs->count] = blockPos;	
	vs->octant[vs->count] = octantMask;
#ifdef STACKSWITCH
	uint switchMask = octantMask ^ vs->octant[vs->count - 1];			
	vs->xSwitch |= (switchMask & XMASK) << vs->count;
#endif
	++vs->count;
#ifdef STACKSWITCH
	vs->ySwitch |= (switchMask & YMASK) << (vs->count);
	vs->zSwitch |= (switchMask & ZMASK) << (vs->count + 1);
#endif
}

inline BlockInfo popVoxel(VoxelStack* vs)
{
	BlockInfo bi;	
	--vs->count;
	bi.blockPos = vs->blockPos[vs->count];		
	bi.octantMask = vs->octant[vs->count];		
	return bi;
}

inline BlockInfo peekVoxel(VoxelStack* vs, int index)
{
	BlockInfo bi;
	bi.blockPos = vs->blockPos[index];
	bi.octantMask = vs->octant[index];
	return bi;
}

#ifdef STACKSWITCH
inline BlockInfo popToSwitch(VoxelStack* vs, uint octantMask)
{	
	uint x, y, z;
	
	x = (octantMask & XMASK) * (32 - clz(vs->xSwitch));
	y = (octantMask & YMASK) * (32 - clz(vs->ySwitch));
	z = (octantMask & ZMASK) * (32 - clz(vs->zSwitch));

	vs->count = max(x, max(y, z));
	uint clearMask = (2 << (vs->count)) - 1;

	vs->xSwitch &= clearMask;
	vs->zSwitch &= clearMask;
	vs->ySwitch &= clearMask;

	return peekVoxel(vs, vs->count);
}
#endif



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

inline uint getAndReduceOctantf(float4* position, float4* dimensions)
{
#ifdef HOSTINCLUDE
	(*dimensions).s0 /= 2.0f;
	(*dimensions).s1 /= 2.0f;
	(*dimensions).s2 /= 2.0f;
#else
	(*dimensions) *= (float4)(0.5f, 0.5f, 0.5f, 0.5f);
#endif

	int4 octants;

#ifdef HOSTINCLUDE
	octants.s3 += isgreater((*position).s0, (*dimensions).s0) + isgreater((*position).s1, (*dimensions).s1) * YMASK + isgreater((*position).s2, (*dimensions).s2) * ZMASK;
#else
	octants = (*position >= *dimensions) * OCTANTMASKS;
	octants.w = -octants.x - octants.y - octants.z;
#endif

#ifdef HOSTINCLUDE
	(*position).s0 = fmodf((*position).s0, (*dimensions).s0);
	(*position).s1 = fmodf((*position).s1, (*dimensions).s1);
	(*position).s2 = fmodf((*position).s2, (*dimensions).s2);
#else
	(*position) = fmod(*position, *dimensions);
#endif
		
	return (uint)octants.s3;
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
	uint normal;
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