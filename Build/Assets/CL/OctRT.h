
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

#ifndef HOSTINCLUDE

uint rayTrace2(/*float4 intersectionPoint,*/__global Block* input, Ray* r, uint4 size, __global Counters* counters, uint maxLOD, float* t)
{
	int rayOctantMask = 0;
	int4 octants = (r->direction < FZEROS) * OCTANTMASKS;
	rayOctantMask -= octants.x + octants.y + octants.z;
	uint maxDepth = min(maxLOD, size.w);
	float4 stepSize = sign(r->direction);	
	float4 fSize = (float4)(size.x, size.y, size.z, size.w);
	float4 initialPosition = fmod(r->origin * fSize * stepSize + fSize, fSize);
	float4 rayDirection;
	VoxelStack vs;
	initStack(&vs, size.s0);

	uint iter = 0;

	float4 relativeSize = fSize;	
	float4 relativeCoords = initialPosition;

	uint curPos = 0;
	__global Block* current = input;

	rayDirection = max(fabs(r->direction), (float4)(1e-37f, 1e-37f, 1e-37f, 0.0f));
	float4 tDelta = 1.0f / rayDirection;

	uint octant;

	pushVoxel(&vs, curPos, 7 ^ rayOctantMask + 8);	//root voxel

	while (vs.count < maxDepth)
	{
		octant = getAndReduceOctantf(&relativeCoords, &relativeSize) ^ rayOctantMask;
		current = input + curPos;

		curPos += getChildPtr(current);
		curPos += octant;
		pushVoxel(&vs, curPos, octant);	

		if (!getValid(current, octant))
			break;		

		++iter;
	}

	if (vs.count == maxDepth && getValid(current, octant))
	{
#ifdef PERFCOUNTERENABLED
		atom_add(&counters->total, iter);
#endif
		return curPos;
	}

	BlockInfo bi;
	*t = 0.0f;

	float4 lastPosition = initialPosition;

	while (iter < 512)
	{

		//traversal
		float4 tMax;
		float4 sideLength;
		float deltaT;
		int stepMask;

		sideLength = fSize * pow(0.5f, vs.count - 1);
		tMax = sideLength - fmod(lastPosition, sideLength);
		tMax *= tDelta;
		deltaT = min(tMax.x, min(tMax.y, tMax.z));
		*t += deltaT;
		lastPosition = initialPosition + rayDirection * (*t);

		stepMask = isequal(deltaT, tMax.x) + isequal(deltaT, tMax.y) * YMASK + isequal(deltaT, tMax.z) * ZMASK;		

		//ascent

#ifdef STACKSWITCH

		if ((peekVoxel(&vs, vs.count - 1).octantMask ^ rayOctantMask) & stepMask)
		{
			bi = popToSwitch(&vs, stepMask);
		}
		else
			bi = popVoxel(&vs);

#else

		bi = popVoxel(&vs);

		while (((bi.octantMask ^ rayOctantMask) & stepMask) != 0)	
		{
			bi = popVoxel(&vs);
			if (bi.blockPos == 0)	//root voxel
			{
#ifdef PERFCOUNTERENABLED
				atom_add(&counters->total, iter);
#endif
				return HITORMISSBIT;
			}
			++iter;
		}

#endif

		if (curPos > 2480)
			return HITORMISSBIT;

		current = input + peekVoxel(&vs, vs.count - 1).blockPos;
		octant = bi.octantMask ^ stepMask;
		curPos = bi.blockPos - bi.octantMask + octant;		
		pushVoxel(&vs, curPos, octant);

		if (!getValid(current, octant))		
			continue;

		current = input + curPos;

		//descent

		relativeSize = fSize * pow(0.5f, vs.count - 1);
		relativeCoords = fmod(lastPosition, relativeSize);
		uint depth = vs.count;

		while (depth < maxDepth)
		{

			curPos += getChildPtr(current);
			octant = getAndReduceOctantf(&relativeCoords, &relativeSize) ^ rayOctantMask;	

			curPos += octant;
			pushVoxel(&vs, curPos, octant);

			if (!getValid(current, octant))
				break;	//no need to go to next level			
		
			current = input + curPos;
			++iter;
			++depth;
		}

		if (depth == maxDepth)
		{
#ifdef PERFCOUNTERENABLED
			atom_add(&counters->total, iter);
#endif
			return curPos;
		}
		++iter;
	}
	
	return curPos;
}

#endif

uint rayTrace(__global Block* input, Ray* r, uint4 size, __global Counters* counters, uint maxLOD)
{

	uint4 startPoint;
	VoxelStack vs;

	initStack(&vs, size.s0);

	float4 intersectionPoint = r->origin;

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
#ifdef HOSTINCLUDE
	stepSize.s0 = sign(r->direction.s0);
	stepSize.s1 = sign(r->direction.s1);
	stepSize.s2 = sign(r->direction.s2);
#else
	stepSize = sign(r->direction);
#endif

	uint4 stepFlag;
	stepFlag.s0 = (stepSize.s0 * 0.5f + 0.5f);
	stepFlag.s1 = (stepSize.s1 * 0.5f + 0.5f);
	stepFlag.s2 = (stepSize.s2 * 0.5f + 0.5f);

	float4 tOffset;

	int zero;

#ifdef HOSTINCLUDE
	zero = (int)ceil(fabs(r->direction.s0 - 0.0));
	r->direction.s0 += (!zero) * FUDGE;
	tOffset.s0 = (!zero) * 1e38;
	zero = (int)ceil(fabs(r->direction.s1 - 0.0));
	r->direction.s1 += (!zero) * FUDGE;
	tOffset.s1 = (!zero) * 1e38;
	zero = (int)ceil(fabs(r->direction.s2 - 0.0));
	r->direction.s2 += (!zero) * FUDGE;	//to prevent 0.0 / 0.0
	tOffset.s2 = (!zero) * 1e38;	
#else
	zero = (int)ceil(fabs(r->direction.s0));
	r->direction.s0 += (!zero) * FUDGE;
	tOffset.s0 = (!zero) * 1e38f;
	zero = (int)ceil(fabs(r->direction.s1));
	r->direction.s1 += (!zero) * FUDGE;
	tOffset.s1 = (!zero) * 1e38f;
	zero = (int)ceil(fabs(r->direction.s2));
	r->direction.s2 += (!zero) * FUDGE;	//to prevent 0.0 / 0.0
	tOffset.s2 = (!zero) * 1e38f;
#endif

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

	uint4 voxelDist;	
	uint sideLength;
#ifdef HOSTINCLUDE
	uint4 ones = {{1, 1, 1, 1}};
	uint4 zeros = {{0, 0, 0, 0}};
#else
	uint4 ones = (uint4)(1, 1, 1, 1);
	uint4 zeros = (uint4)(0, 0, 0, 0);
#endif

	while(iter < 512)
	{
		bi = popVoxel(&vs);

		sideLength = size.s0 >> vs.count + 1;

#ifdef HOSTINCLUDE
		
		voxelDist.s0 = startPoint.s0 % sideLength;
		voxelDist.s0 = (1 - stepFlag.s0) * voxelDist.s0 + (stepFlag.s0) * (sideLength - voxelDist.s0 - 1);

		voxelDist.s1 = startPoint.s1 % sideLength;
		voxelDist.s1 = (1 - stepFlag.s1) * voxelDist.s1 + (stepFlag.s1) * (sideLength - voxelDist.s1 - 1);

		voxelDist.s2 = startPoint.s2 % sideLength;
		voxelDist.s2 = (1 - stepFlag.s2) * voxelDist.s2 + (stepFlag.s2) * (sideLength - voxelDist.s2 - 1);		

#else
		voxelDist = startPoint % (uint4)(sideLength, sideLength, sideLength, 0);
		voxelDist = (ones - stepFlag) * voxelDist + stepFlag * ((uint4)(sideLength, sideLength, sideLength, 0) - voxelDist - ones);
#endif

		float4 tMax;
		float4 nextVoxel;
		float t = 0.0f;

		nextVoxel.s0 = (startPoint.s0 - intersectionPoint.s0) * stepSize.s0 + stepFlag.s0;	//distance to nearest voxel boundary at highest LOD
		nextVoxel.s1 = (startPoint.s1 - intersectionPoint.s1) * stepSize.s1 + stepFlag.s1;
		nextVoxel.s2 = (startPoint.s2 - intersectionPoint.s2) * stepSize.s2 + stepFlag.s2;

		nextVoxel.s0 += voxelDist.s0;
		nextVoxel.s1 += voxelDist.s1;
		nextVoxel.s2 += voxelDist.s2;		

#ifdef HOSTINCLUDE
		tMax.s0 = tOffset.s0 + nextVoxel.s0 / (r->direction.s0 * stepSize.s0);
		tMax.s1 = tOffset.s1 + nextVoxel.s1 / (r->direction.s1 * stepSize.s1);
		tMax.s2 = tOffset.s2 + nextVoxel.s2 / (r->direction.s2 * stepSize.s2);
#else
		tMax = tOffset + nextVoxel / (r->direction * stepSize);
#endif

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
		else
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

		oldStartPoint.s0 = abs(oldStartPoint.s0 - (int)startPoint.s0) > voxelDist.s0;
		oldStartPoint.s1 = abs(oldStartPoint.s1 - (int)startPoint.s1) > voxelDist.s1;
		oldStartPoint.s2 = abs(oldStartPoint.s2 - (int)startPoint.s2) > voxelDist.s2;
	
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
		
		if (!isEmpty(&vs))
		{
			curPos = peekVoxel(&vs, vs.count - 1).blockPos;
			current = input + curPos;
			curPos += getChildPtr(current);
		}
		else
		{
			curPos = 0;
			current = 0;
		}

		depth = vs.count;

#ifdef HOSTINCLUDE
		relativeSize.s0 = size.s0 >> depth;
		relativeSize.s1 = size.s1 >> depth;
		relativeSize.s2 = size.s2 >> depth;		

		relativeCoords.s0 = startPoint.s0 % relativeSize.s0;
		relativeCoords.s1 = startPoint.s1 % relativeSize.s1;
		relativeCoords.s2 = startPoint.s2 % relativeSize.s2;
#else
		relativeSize = rotate(size, (uint4)(32 - depth, 32 - depth, 32 - depth, 32 - depth));
		relativeCoords = startPoint % relativeSize;
#endif

		uint nextPos = 0;

		while (depth < maxDepth && iter < 512)
		{
			octant = getAndReduceOctant(&relativeCoords, &relativeSize);
			curPos += nextPos;
			curPos += octant;
			pushVoxel(&vs, curPos, octant);

			if (current)
			{
				if (!getValid(current, octant))
					break;	//no need to go to next level
			}
		
			current = input + curPos;
			nextPos = getChildPtr(current);
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