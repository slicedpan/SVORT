#include "Octree.h"

typedef struct 
{
	float4 origin;
	float4 direction;
} ray;

typedef struct 
{	
	float16 invWorldView;
	uint4 size;
	float4 invSize;
} Params;

typedef struct
{
	uint numSamples;
	uint total;
} Counters;

typedef struct
{
	uint count;
	Block* block[15];
}	VoxelStack;

void initStack(VoxelStack* vs)
{
	vs->count = 0;
}

void pushVoxel(VoxelStack* vs, Block* b)
{
	vs->block[vs->count] = b;
	++vs->count;
}

Block* popVoxel(VoxelStack* vs)
{
	--vs->count;
	return vs->block[vs->count + 1];
}

float4 multVecMat(const float4* vector, __constant float16* matrix)
{
	float4 retVec;
	__constant float4* matPtr = (__constant float4*)matrix;
	retVec.x = dot(*matPtr, *vector);
	retVec.y = dot(*(matPtr + 1), *vector);
	retVec.z = dot(*(matPtr + 2), *vector);
	retVec.w = dot(*(matPtr + 3), *vector);
	
	return retVec;
}

float4 multMatVec(const float4* vector, __constant float16* matrix)
{
	float4 retVec;
	float4 tmp;
	tmp.x = (*matrix).s0;
	tmp.y = (*matrix).s4;
	tmp.z = (*matrix).s8;
	tmp.w = (*matrix).sC;
	retVec.x = dot(tmp, *vector);
	
	tmp.x = (*matrix).s1;
	tmp.y = (*matrix).s5;
	tmp.z = (*matrix).s9;
	tmp.w = (*matrix).sD;
	retVec.y = dot(tmp, *vector);

	tmp.x = (*matrix).s2;
	tmp.y = (*matrix).s6;
	tmp.z = (*matrix).sA;
	tmp.w = (*matrix).sE;
	retVec.z = dot(tmp, *vector);

	tmp.x = (*matrix).s3;
	tmp.y = (*matrix).s7;
	tmp.z = (*matrix).sB;
	tmp.w = (*matrix).sF;
	retVec.w = dot(tmp, *vector);
	
	return retVec;
}

bool intersectSphere(ray r, float radius, float4* intersectionPoint)
{
	float4 endPoint = r.origin + (1000.0 * r.direction);
	float dist = length(endPoint.xyz - r.origin.xyz);
	float4 tmp1 = - r.origin;
	float4 tmp2 = endPoint - r.origin;
	tmp1 *= tmp2;
	float u = tmp1.x + tmp1.y + tmp1.z;
	u /= dist * dist;
	
	if (u < 0.0  || u > 1.0)
		return false;
	*intersectionPoint = r.origin + u * tmp2;
	if (length((*intersectionPoint).xyz) < radius)
		return true;
	return false;
}

bool intersectCube(ray r, float t0, float t1, float4* intersectionPoint)
{
	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	if (r.direction.x >= 0) 
	{
		tmin = (- r.origin.x) / r.direction.x;
		tmax = (1 - r.origin.x) / r.direction.x;
	}
	else 
	{
		tmin = (1 - r.origin.x) / r.direction.x;
		tmax = (- r.origin.x) / r.direction.x;
	}
	if (r.direction.y >= 0) 
	{
		tymin = (- r.origin.y) / r.direction.y;
		tymax = (1 - r.origin.y) / r.direction.y;
	}
	else 
	{
		tymin = (1 - r.origin.y) / r.direction.y;
		tymax = (- r.origin.y) / r.direction.y;
	}	
	if ( (tmin > tymax) || (tymin > tmax) )
		return false;	

	if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;
	if (r.direction.z >= 0) 
	{
		tzmin = (- r.origin.z) / r.direction.z;
		tzmax = (1 - r.origin.z) / r.direction.z;
	}
	else 
	{
		tzmin = (1 - r.origin.z) / r.direction.z;
		tzmax = (- r.origin.z) / r.direction.z;
	}
	if ( (tmin > tzmax) || (tzmin > tmax) )
		return false;
	if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;
	(*intersectionPoint).xyz = r.origin.xyz + tmin * r.direction.xyz;
	return ( (tmin < t1) && (tmax > t0) );
}