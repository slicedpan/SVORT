
#ifndef _OCTRT_H
#define _OCTRT_H

#include "RT.h"
#include "Octree.h"

#define HITORMISSBIT	2147483648	//2^31
#define POSMASK 2147483647	//(2^31) - 1
#define FUDGE 1e-7f
#define counterVar iter

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

uint castRay(uint4 startVoxel, float4 intersectionPoint, __global Block* input, VoxelStack* vs, Ray* r, uint4 size, __global Counters* counters, uint maxLOD)
{

	int3 stepSize;

	stepSize.s0 = sign(r->direction.s0);
	stepSize.s1 = sign(r->direction.s1);
	stepSize.s2 = sign(r->direction.s2);

	float3 stepFlag;
	stepFlag.s0 = ((float)stepSize.s0 * 0.5) + 0.5;
	stepFlag.s1 = ((float)stepSize.s1 * 0.5) + 0.5;
	stepFlag.s2 = ((float)stepSize.s2 * 0.5) + 0.5;

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
	

	uint iter = 0;

	__global Block* blockPtr;
	BlockInfo bi;

	while(iter < 512)
	{			
		uint nextOctant;		
		bi = popVoxel(vs);	 

		if(tMax.s0 < tMax.s2) 
		{
			if(tMax.s0 < tMax.s1) 
			{				
				tMax.s0 = tMax.s0 + tDelta.s0;
				while (bi.octantMask & XMASK == stepSize.s0)
				{
					if (isEmpty(vs))
						return HITORMISSBIT;	//there are no further voxels in the x-direction
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
						return HITORMISSBIT;
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
						return HITORMISSBIT;
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
						return HITORMISSBIT;
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
				
			}
		}		
	}

#ifdef PERFCOUNTERENABLED
	if (counters)
		atom_add(&counters->total, iter);
#endif

	return 0;
}

uint rayTrace(float4 intersectionPoint, __global Block* input, Ray* r, uint4 size, __global Counters* counters, uint maxLOD)
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

	float4 tOffset;

	int zero = (int)ceil(fabs(r->direction.s0 - 0.0));
	r->direction.s0 += (!zero) * FUDGE;
	tOffset.s0 = (!zero) * 1e126;
	zero = (int)ceil(fabs(r->direction.s1 - 0.0));
	r->direction.s1 += (!zero) * FUDGE;
	tOffset.s1 = (!zero) * 1e126;
	zero = (int)ceil(fabs(r->direction.s2 - 0.0));
	r->direction.s2 += (!zero) * FUDGE;	//to prevent 0.0 / 0.0
	tOffset.s2 = (!zero) * 1e126;

	uint rayOctantMask = 0;
	if (r->direction.s0 > 0.0)
		rayOctantMask |= XMASK;
	if (r->direction.s1 > 0.0)
		rayOctantMask |= YMASK;
	if (r->direction.s2 > 0.0)
		rayOctantMask |= ZMASK;

	uint depth = 0;
	uint maxDepth = min(maxLOD, size.s3);

	uint4 relativeSize = size;
	uint4 relativeCoords = startPoint;

	uint curPos = 0;
	__global Block* current = 0;

	uint octant;
	uint iter = 0;
	uint initialDescent = 0;
	uint descent = 0;
	uint ascent = 0;
	uint traversal = 0;
	uint escapes = 0;

	while (depth < maxDepth)
	{
		octant = getAndReduceOctant(&relativeCoords, &relativeSize);

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
		++iter;
	}

	if (depth == maxDepth)
	{
#ifdef PERFCOUNTERENABLED
		atom_add(&counters->total, counterVar);
#endif
		return curPos;
	}	

	BlockInfo bi;

	int4 voxelDist;	
	uint sideLength;

	while(iter < 64)
	{
		bi = popVoxel(&vs);

		sideLength = size.s0 >> vs.count + 1;
		
		voxelDist.s0 = startPoint.s0 % sideLength;
		voxelDist.s0 = (1 - stepFlag.s0) * voxelDist.s0 + (stepFlag.s0) * (sideLength - voxelDist.s0 - 1);

		voxelDist.s1 = startPoint.s1 % sideLength;
		voxelDist.s1 = (1 - stepFlag.s1) * voxelDist.s1 + (stepFlag.s1) * (sideLength - voxelDist.s1 - 1);

		voxelDist.s2 = startPoint.s2 % sideLength;
		voxelDist.s2 = (1 - stepFlag.s2) * voxelDist.s2 + (stepFlag.s2) * (sideLength - voxelDist.s2 - 1);		

		float4 tMax;
		float4 nextVoxel;
		float t = 0.0;

		nextVoxel.s0 = (startPoint.s0 - intersectionPoint.s0) * stepSize.s0 + stepFlag.s0;	//distance to nearest voxel boundary at highest LOD
		nextVoxel.s1 = (startPoint.s1 - intersectionPoint.s1) * stepSize.s1 + stepFlag.s1;
		nextVoxel.s2 = (startPoint.s2 - intersectionPoint.s2) * stepSize.s2 + stepFlag.s2;

		nextVoxel.s0 += voxelDist.s0;
		nextVoxel.s1 += voxelDist.s1;
		nextVoxel.s2 += voxelDist.s2;		

		tMax.s0 = tOffset.s0 + nextVoxel.s0 / (r->direction.s0 * stepSize.s0);
		tMax.s1 = tOffset.s1 + nextVoxel.s1 / (r->direction.s1 * stepSize.s1);
		tMax.s2 = tOffset.s2 + nextVoxel.s2 / (r->direction.s2 * stepSize.s2);

		t = min(tMax.s0, min(tMax.s1, tMax.s2));

		int4 oldStartPoint;
		oldStartPoint.s0 = startPoint.s0;
		oldStartPoint.s1 = startPoint.s1;
		oldStartPoint.s2 = startPoint.s2;
	
		if (tMax.s0 == t)
		{
			//X				
			intersectionPoint.s0 += nextVoxel.s0 * stepSize.s0;
			intersectionPoint.s1 += r->direction.s1 * t;
			intersectionPoint.s2 += r->direction.s2 * t;
			zero = (int)ceil(intersectionPoint.s0 - floor(intersectionPoint.s0));
			startPoint.s0 = floor(intersectionPoint.s0);
			startPoint.s1 = floor(intersectionPoint.s1);
			startPoint.s2 = floor(intersectionPoint.s2);
			startPoint.s0 -= (1 - stepFlag.s0) * (!zero);
		}
		else if (tMax.s1 == t)
		{
			//Y
			intersectionPoint.s0 += r->direction.s0 * t;
			intersectionPoint.s1 += nextVoxel.s1 * stepSize.s1;
			intersectionPoint.s2 += r->direction.s2 * t;
			startPoint.s0 = floor(intersectionPoint.s0);
			startPoint.s1 = floor(intersectionPoint.s1);
			startPoint.s2 = floor(intersectionPoint.s2);
			zero = (int)ceil(intersectionPoint.s1 - floor(intersectionPoint.s1));
			startPoint.s1 -= (1 - stepFlag.s1) * (!zero);
		}
		else if (tMax.s2 == t)
		{
			//Z
			intersectionPoint.s0 += r->direction.s0 * t;
			intersectionPoint.s1 += r->direction.s1 * t;
			intersectionPoint.s2 += nextVoxel.s2 * stepSize.s2;
			zero = (int)ceil(intersectionPoint.s2 - floor(intersectionPoint.s2));
			startPoint.s0 = floor(intersectionPoint.s0);
			startPoint.s1 = floor(intersectionPoint.s1);
			startPoint.s2 = floor(intersectionPoint.s2);
			startPoint.s2 -= (1 - stepFlag.s2) * (!zero);
		}		

		oldStartPoint.s0 = abs(oldStartPoint.s0 - (int)startPoint.s0);
		oldStartPoint.s1 = abs(oldStartPoint.s1 - (int)startPoint.s1);
		oldStartPoint.s2 = abs(oldStartPoint.s2 - (int)startPoint.s2);
	
		if (oldStartPoint.s0)
		{
			while ((bi.octantMask & XMASK) == stepFlag.s0)
			{
				if (isEmpty(&vs))
				{
					++escapes;
#ifdef PERFCOUNTERENABLED
					atom_add(&counters->total, counterVar);
#endif
					return HITORMISSBIT;
				}
				bi = popVoxel(&vs);
				++ascent;
			}
		}
		
		if (oldStartPoint.s1)
		{
			while ((bi.octantMask & YMASK) == stepFlag.s1 << 1)
			{
				if (isEmpty(&vs))
				{
					++escapes;
#ifdef PERFCOUNTERENABLED
					atom_add(&counters->total, counterVar);
#endif
					return HITORMISSBIT;
				}
				bi = popVoxel(&vs);
				++ascent;
			}
		}

		if (oldStartPoint.s2)
		{
			while ((bi.octantMask & ZMASK) == stepFlag.s2 << 2)
			{
				if (isEmpty(&vs))
				{
					++escapes;
#ifdef PERFCOUNTERENABLED
					atom_add(&counters->total, counterVar);
#endif
					return HITORMISSBIT;				
				}
				bi = popVoxel(&vs);
				++ascent;
			}
		}
		
		if (startPoint.s0 >= size.s0)
		{
			++escapes;
#ifdef PERFCOUNTERENABLED
					atom_add(&counters->total, counterVar);
#endif
			return HITORMISSBIT;
		}
		if (startPoint.s1 >= size.s1) 
		{
			++escapes;
#ifdef PERFCOUNTERENABLED
					atom_add(&counters->total, counterVar);
#endif
			return HITORMISSBIT;
		}
		if (startPoint.s2 >= size.s2)
		{
			++escapes;
#ifdef PERFCOUNTERENABLED
					atom_add(&counters->total, counterVar);
#endif
			return HITORMISSBIT;
		}

		curPos = peekVoxel(&vs, vs.count - 1).blockPos;
		if (curPos)
		{
			current = input + curPos;
			curPos += getChildPtr(current);
		}
		else
		{
			current = 0;
		}

		depth = vs.count;

		relativeSize.s0 = size.s0 >> depth;
		relativeSize.s1 = size.s1 >> depth;
		relativeSize.s2 = size.s2 >> depth;		

		relativeCoords.s0 = startPoint.s0 % relativeSize.s0;
		relativeCoords.s1 = startPoint.s1 % relativeSize.s1;
		relativeCoords.s2 = startPoint.s2 % relativeSize.s2;

		while (depth < maxDepth && iter < 512)
		{
			octant = getAndReduceOctant(&relativeCoords, &relativeSize);

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
			++iter;
			++descent;
		}

		if (depth == maxDepth && getValid(current, octant))
		{
#ifdef PERFCOUNTERENABLED
			atom_add(&counters->total, counterVar);
#endif
			return curPos;
		}
		++traversal;
		++iter;
	}


	
#ifdef PERFCOUNTERENABLED
		atom_add(&counters->total, 1);
#endif
	return 0;
	//return HITORMISSBIT;

}



#endif