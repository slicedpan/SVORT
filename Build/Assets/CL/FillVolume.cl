#include "colour.h"
#include "grid.h"

__constant const sampler_t sampler2D = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP;

__kernel void FillVolume(image2d_t input, image2d_t normals, __global uint2* output, uint4 sizeAndLayer, __global float4* normalLookup)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	uint3 coords = (uint3)(x, y, sizeAndLayer.w);
	
	float4 colour = read_imagef(input, sampler2D, (int2)(x, y));
	float4 normal = read_imagef(normals, sampler2D, (int2)(x, y));	
	
	output[GetGridOffset(coords, sizeAndLayer.xyz)].s0 = PackColour(colour);
	output[GetGridOffset(coords, sizeAndLayer.xyz)].s1 = PackColour(normal);
	
}