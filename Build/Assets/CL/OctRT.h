#include "CLDefsBegin.h"
#include "RT.h"
#include "Octree.h"

uint findStartPoint(uint3* startPoint, __global Block* input, VoxelStack* vs, __constant Params* params, __global Counters* counters, uint maxLOD)
{
	uint curPos = 0;
	uint3 size;

	size.s0 = params->size.s0;
	size.s1 = params->size.s1;
	size.s2 = params->size.s2;

	bool hit = 0;
	uint iter = 0;
	while (iter < maxLOD && !hit)
	{	
		uint octant = getOctant(*startPoint, size);
		
		if (curPos)
		{
			if (!getValid(input + curPos, octant))
			{	
				hit = 1;		
			}
		}
		curPos += octant;
		pushVoxel(vs, curPos, octant);		
		uint childOffset = getChildPtr(input + curPos);
		curPos += childOffset;
		if (size.s0 == 2)
		{
			curPos |= 2147483648;
			break;
		}
		reduceOctant(startPoint, &size);

		++iter;
	}
	if (counters)
		atom_add(&counters->total, iter);
	return curPos;
}

uint castRay(uint3* startPoint, __global Block* input, VoxelStack* vs, Ray* r, __constant Params* params, __global Counters* counters, uint maxLOD)
{
	uint rayOctantMask = 0;

	if (r->direction.s0 > 0.0)
		rayOctantMask |= 1;
	if (r->direction.s1 > 0.0)
		rayOctantMask |= 2;
	if (r->direction.s2 > 0.0)
		rayOctantMask |= 4;

	while(1)
	{
		uint3 currentSize;
		currentSize.s0 = params->size.s0 >> vs->count;
		currentSize.s1 = params->size.s1 >> vs->count;
		currentSize.s2 = params->size.s2 >> vs->count;
		BlockInfo bi = popVoxel(vs);
		break;
	}
	return 0;
}

#include "CLDefsEnd.h"