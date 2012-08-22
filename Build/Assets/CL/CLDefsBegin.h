#ifdef _WIN32
#pragma once

#include <CL\cl.h>

#define uint cl_uint
#define uint2 cl_uint2
#define int3 cl_int3
#define uint3 cl_uint3
#define uint4 cl_uint4
#define float16 cl_float16
#define float4 cl_float4
#define float3 cl_float3
#define int4 cl_int4
#define ushort2 cl_ushort2
#define ushort4 cl_ushort4
#define short4 cl_short4

#define s0 s[0]
#define s1 s[1]
#define s2 s[2]
#define s3 s[3]

#define __global
#define __constant

#define HOSTINCLUDE

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

template <typename T>
int isgreater(T x, T y)
{
	return x > y;
}

template <typename T>
inline T min(T x, T y)
{
	return (x < y) ? x : y;
}

inline float max(float x, float y)
{
	return (x > y) ? x : y;
}

namespace SVO
{
#endif