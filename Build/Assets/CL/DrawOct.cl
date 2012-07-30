#include "Octree.h"
#include "colour.h"
#include "grid.h"

__kernel void DrawOctree(__write_only image2d_t output, __global Block* input, uint4 mipOffset, uint4 sizeAndLayer)
{
	int2 screenCoords = (int2)(get_global_id(0), get_global_id(1));
	uint2 dimensions = (uint2)(get_image_width(output), get_image_height(output));
		
	uint3 coords;

	coords.x = sizeAndLayer.x * ((float)screenCoords.x / (float)(dimensions.x - 1));
	coords.y = sizeAndLayer.y * ((float)screenCoords.y / (float)(dimensions.y - 1));
	coords.z = sizeAndLayer.w;
	
	uint3 size = (uint3)(sizeAndLayer.x, sizeAndLayer.y, sizeAndLayer.z);
	uint mipLevel = mipOffset.z % 16;	
	
	float4 colour;
	
	__global Block* current = 0;
	uint curPos = 0;

	uint2 pixSize = dimensions;
	uint childOffset = 0;
	for (int i = 0; i <= mipLevel; ++i)
	{		
		pixSize /= 2;
		uint octant = getOctant(coords, size);
		if (current)
		{
			if (!getValid(current, octant))
			{	
				i = 16;			
			}
		}
		curPos += octant;
		current = input + curPos;		
		childOffset = getChildPtr(current);
		if (!childOffset)
			break;
		curPos += childOffset;		
		reduceOctant(&coords, &size);
		
	}
	
	colour = UnpackColour(current->colour);
	
	//colour.x = 1.0;	
	
	if (colour.w == 0.0)
		colour.x = 1.0;

	if ((screenCoords.x % pixSize.x) == 0 || (screenCoords.y % pixSize.y) == 0)
		colour.xyz = (float3)(1.0, 1.0, 1.0);
	
	write_imagef(output, screenCoords, colour);
	
}