
#ifndef _OCTRT_H
#define _OCTRT_H

#include "RT.h"
#include "Octree.h"

#define HITORMISSBIT	2147483648	//2^31
#define POSMASK 2147483647	//(2^31) - 1

uint findStartPoint(uint4 startPoint, __global Block* input, VoxelStack* vs, uint4 size, __global Counters* counters, uint maxLOD)
{
	uint curPos = 0;

	bool hit = 0;
	uint iter = 0;

	__global Block* current = 0;

	while (iter < maxLOD && !hit)
	{	
		uint octant = getAndReduceOctant(&startPoint, &size);
		
		if (current)
		{
			if (!getValid(current, octant))
			{	
				hit = 1;		
			}
		}
		curPos += octant;
		pushVoxel(vs, curPos, octant);
		current = input + curPos;
		uint childOffset = getChildPtr(input + curPos);
		curPos += childOffset;
		if (size.s0 == 1)
		{			
			curPos |= 2147483648;
			break;
		}
		++iter;
	}
#ifdef PERFCOUNTERENABLED
	if (counters)
		atom_add(&counters->total, iter);
#endif
	return curPos;
}

uint descendHierarchy(uint4 voxelCoord, __global Block* input, VoxelStack* vs, uint maxDepth, uint4 size)
{
	uint curDepth = vs->count;
	__global Block* parent;
	return 0;
	while (vs->count != maxDepth)
	{
		
	}
	
}

uint castRay(uint4 startVoxel, float3 intersectionPoint, __global Block* input, VoxelStack* vs, Ray* r, uint4 size, __global Counters* counters, uint maxLOD)
{

	int3 stepSize;

	stepSize.s0 = sign(r->direction.s0);
	stepSize.s1 = sign(r->direction.s1);
	stepSize.s2 = sign(r->direction.s2);

	float3 stepFlag;
	stepFlag.s0 = ((float)stepSize.s0 * 0.5) + 0.5;
	stepFlag.s1 = ((float)stepSize.s1 * 0.5) + 0.5;
	stepFlag.s2 = ((float)stepSize.s2 * 0.5) + 0.5;

	uint3 maxCoord;

	uint maxDepth = min(maxLOD, size.s3);

	float3 tDelta;
	tDelta.s0 = fabs(1.0 / r->direction.s0);
	tDelta.s1 = fabs(1.0 / r->direction.s1);
	tDelta.s2 = fabs(1.0 / r->direction.s2);		 

	float3 tMax;
	tMax.s0 = (startVoxel.s0 - intersectionPoint.s0 + stepFlag.s0);	
	tMax.s1 = (startVoxel.s1 - intersectionPoint.s1 + stepFlag.s1);
	tMax.s2 = (startVoxel.s2 - intersectionPoint.s2 + stepFlag.s2);	

	tMax.s0 /= r->direction.s0;
	tMax.s1 /= r->direction.s1;
	tMax.s2 /= r->direction.s2;

	maxCoord.s0 = (size.s0 + (stepSize.s0 * size.s0)) / 2 + stepSize.s0 * 0.5 - 0.5;	//branchless, set to minus one if stepSize is negative
	maxCoord.s1 = (size.s1 + (stepSize.s1 * size.s1)) / 2 + stepSize.s1 * 0.5 - 0.5;
	maxCoord.s2 = (size.s2 + (stepSize.s2 * size.s2)) / 2 + stepSize.s2 * 0.5 - 0.5;

	uint iter = 0;

	__global Block* blockPtr;
	BlockInfo bi;

	while(iter < 512)
	{			
		uint nextOctant;		
		bi = popVoxel(vs);
		uint sideLength = size.s0 >> vs->count;			 

		if(tMax.s0 < tMax.s2) 
		{
			if(tMax.s0 < tMax.s1) 
			{				
				tMax.s0 = tMax.s0 + tDelta.s0;
				while (bi.octantMask & XMASK == stepSize.s0)
				{
					if (isEmpty(vs))
						return 1 << 31;	//there are no further voxels in the x-direction
					bi = popVoxel(vs);	
				}
				nextOctant = bi.octantMask + XMASK;
				pushVoxel(vs, bi.blockPos + XMASK, nextOctant);
				startVoxel.s0 += stepSize.s0;
			} 
			else 
			{
				tMax.s1 = tMax.s1 + tDelta.s1;
				while (bi.octantMask & YMASK == stepSize.s1)
				{
					if (isEmpty(vs))
						return 1 << 31;
					bi = popVoxel(vs);
				}		
				nextOctant = bi.octantMask + YMASK;
				pushVoxel(vs, bi.blockPos + YMASK, nextOctant);
				startVoxel.s1 += stepSize.s1;
			}
		} 
		else 
		{
			if(tMax.s1 < tMax.s2) 
			{			
				tMax.s1 = tMax.s1 + tDelta.s1;
				while (bi.octantMask & YMASK == stepSize.s1)
				{
					if (isEmpty(vs))
						return 1 << 31;
					bi = popVoxel(vs);
				}		
				nextOctant = bi.octantMask + YMASK;
				pushVoxel(vs, bi.blockPos + YMASK, nextOctant);
				startVoxel.s1 += stepSize.s1;
			}
			else 
			{			
				tMax.s2 = tMax.s2 + tDelta.s2;
				while (bi.octantMask & ZMASK == stepSize.s2)
				{
					if (isEmpty(vs))
						return 1 << 31;
					bi = popVoxel(vs);
				}			
				nextOctant = bi.octantMask + ZMASK;
				pushVoxel(vs, bi.blockPos + ZMASK, nextOctant);
				startVoxel.s2 += stepSize.s2;
			}
		}

		bi = peekVoxel(vs, vs->count - 2);
		blockPtr = input + bi.blockPos;

		++iter;

		if (!getValid(blockPtr, nextOctant))
		{			
			continue;		
		}
		else
		{
			if (vs->count == maxDepth)
			{
				bi = popVoxel(vs);
				return bi.blockPos;
			}
			else
			{
				if (descendHierarchy(startVoxel, input, vs, maxDepth, size))
				{
					bi = popVoxel(vs);
					return bi.blockPos;
				}
			}
		}		
	}

#ifdef PERFCOUNTERENABLED
	if (counters)
		atom_add(&counters->total, iter);
#endif

	return 0;
}

uint rayTrace(float3 intersectionPoint, __global Block* input, Ray* r, uint4 size, __global Counters* counters, uint maxLOD)
{

	uint4 startPoint;
	VoxelStack vs;

	initStack(&vs, size.s0);

	intersectionPoint.s0 *= size.s0;
	intersectionPoint.s1 *= size.s1;
	intersectionPoint.s2 *= size.s2;

	startPoint.s0 = floor(intersectionPoint.s0);
	startPoint.s1 = floor(intersectionPoint.s1);
	startPoint.s2 = floor(intersectionPoint.s2);

	if (startPoint.s0 == size.s0) --startPoint.s0;	//TODO: make this branchless (if possible)
	if (startPoint.s1 == size.s1) --startPoint.s1;
	if (startPoint.s2 == size.s2) --startPoint.s2;

	float4 stepSize;
	stepSize.s0 = sign(r->direction.s0);
	stepSize.s1 = sign(r->direction.s1);
	stepSize.s2 = sign(r->direction.s2);

	uint4 stepFlag;
	stepFlag.s0 = (stepSize.s0 * 0.5 + 0.5);
	stepFlag.s1 = (stepSize.s1 * 0.5 + 0.5);
	stepFlag.s2 = (stepSize.s2 * 0.5 + 0.5);

	uint depth = 0;
	uint maxDepth = min(maxLOD, size.s3);

	uint4 relativeSize = size;
	uint4 relativeCoords = startPoint;

	uint curPos = 0;
	__global Block* current = 0;	

	while (depth < maxDepth)
	{
		uint octant = getAndReduceOctant(&relativeCoords, &relativeSize);

		curPos += octant;
		pushVoxel(&vs, curPos, octant);

		if (current)
		{
			if (!getValid(current, octant))
				break;	//no need to go to next level
		}
		
		current = input + curPos;
		curPos += getChildPtr(current);
		++depth;	//depth is 0 if the first voxel is not valid
	}

	float3 tDelta;
	tDelta.s0 = fabs(1.0 / r->direction.s0);
	tDelta.s1 = fabs(1.0 / r->direction.s1);
	tDelta.s2 = fabs(1.0 / r->direction.s2);		 

	float3 tMax;
	tMax.s0 = (startPoint.s0 - intersectionPoint.s0 + stepFlag.s0);	
	tMax.s1 = (startPoint.s1 - intersectionPoint.s1 + stepFlag.s1);
	tMax.s2 = (startPoint.s2 - intersectionPoint.s2 + stepFlag.s2);	

	tMax.s0 /= r->direction.s0;
	tMax.s1 /= r->direction.s1;
	tMax.s2 /= r->direction.s2;

	uint4 maxCoord;

	maxCoord.s0 = (size.s0 + (stepSize.s0 * size.s0)) / 2 + stepSize.s0 * 0.5 - 0.5;	//branchless, set to minus one if stepSize is negative
	maxCoord.s1 = (size.s1 + (stepSize.s1 * size.s1)) / 2 + stepSize.s1 * 0.5 - 0.5;
	maxCoord.s2 = (size.s2 + (stepSize.s2 * size.s2)) / 2 + stepSize.s2 * 0.5 - 0.5;

	BlockInfo bi;

	uint4 voxelDist;
	voxelDist.s0 = 0;
	voxelDist.s1 = 0;
	voxelDist.s2 = 0;

	while(1)
	{
		bi = popVoxel(&vs);
		if (vs.count + 1 < size.s3)
		{
			voxelDist.s0 += bi.centre.s0 - startPoint.s0;
			voxelDist.s1 += bi.centre.s1 - startPoint.s1;
			voxelDist.s2 += bi.centre.s2 - startPoint.s2;
		}

		tMax.s0 += voxelDist.s0 / r->direction.s0;
		tMax.s1 += voxelDist.s1 / r->direction.s1;
		tMax.s2 += voxelDist.s2 / r->direction.s2;

		break;

	}
	

	return 0;

}



#endif