
#include "colour.h"
#include "grid.h"

__kernel void DrawVoxels(__write_only image2d_t output, __global int* input, uint4 mipOffset, uint4 sizeAndLayer)
{
	int2 screenCoords = (int2)(get_global_id(0), get_global_id(1));
	uint2 dimensions = (uint2)(get_image_width(output), get_image_height(output));
		
	uint3 coords;
	coords.x = sizeAndLayer.x * ((float)screenCoords.x / (float)(dimensions.x - 1));
	coords.y = sizeAndLayer.y * ((float)screenCoords.y / (float)(dimensions.y - 1));
	coords.z = sizeAndLayer.w;	
	
	uint pos = mipOffset.y + GetGridOffset(coords, sizeAndLayer.xyz);	
	
	//colour.x = 1.0;
	
	float4 colour = UnpackColour(input[pos]);
	
	if (colour.w == 0.0)
		colour.x = 1.0;
	
	write_imagef(output, screenCoords, colour);
	
}