#include "colour.h"
#include "grid.h"

__constant const sampler_t sampler2D = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP;

__kernel void FillVolume(image2d_t input, __global int* output, int4 sizeAndLayer)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	int3 coords = (int3)(x, y, sizeAndLayer.w);
	
	float4 colour = read_imagef(input, sampler2D, coords.xy);	
	
	output[GetGridOffset(coords, sizeAndLayer.xyz)] = PackColour(colour);
	
}