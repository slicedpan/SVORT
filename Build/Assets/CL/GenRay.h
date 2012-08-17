
#ifndef _GENRAY_H
#define _GENRAY_H

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

inline Ray createRay(__constant float16* invWorldView, float2 screenPos)
{
	Ray r;
	r.origin = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
	r.origin = multMatVec(&r.origin, invWorldView);
	r.direction = (float4)(screenPos.x * 2.0f - 1.0f, screenPos.y * 2.0f - 1.0f, -1.0f, 0.0f);
	r.direction = multMatVec(&r.direction, invWorldView);
	r.direction.xyz = normalize(r.direction.xyz);
	return r;
}

#endif