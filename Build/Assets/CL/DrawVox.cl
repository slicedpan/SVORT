
#include "colour.h"
#include "grid.h"

__kernel void DrawVoxels(__write_only image2d_t output, __global int* input, int4 mipOffset, int4 sizeAndLayer)
{
	int2 screenCoords = (int2)(get_global_id(0), get_global_id(1));
	int2 dimensions = (int2)(get_image_width(output), get_image_height(output));
		
	int3 coords;
	coords.x = sizeAndLayer.x * ((float)screenCoords.x / (float)(dimensions.x - 1));
	coords.y = sizeAndLayer.y * ((float)screenCoords.y / (float)(dimensions.y - 1));
	coords.z = sizeAndLayer.w;
	
	int pos = mipOffset.y + GetGridOffset(coords, sizeAndLayer.xyz);	
	
	//colour.x = 1.0;
	
	//colour = (float4)(1.0, 0.0, 0.0, 1.0);
	
	write_imagef(output, screenCoords, UnpackColour(input[pos]));
	
}