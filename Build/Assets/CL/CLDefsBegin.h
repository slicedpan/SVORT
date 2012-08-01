#ifdef _WIN32
#pragma once

#include <CL\cl.h>

#define uint cl_uint
#define int3 cl_int3
#define uint3 cl_uint3
#define uint4 cl_uint4
#define float16 cl_float16
#define float4 cl_float4
#define int4 cl_uint4

#define s0 s[0]
#define s1 s[1]
#define s2 s[2]
#define s3 s[3]

#define __global
#define __constant

inline float dot(float4 u, float4 v)
{
	return u.s0 * v.s0 + u.s1 * v.s1 + u.s2 * v.s2 + u.s3 * v.s3;		 
}

inline uint atom_add(uint* i, uint val)
{
	uint old;
	old = *i;
	*i += val;
	return old;
}

namespace SVO
{
#endif