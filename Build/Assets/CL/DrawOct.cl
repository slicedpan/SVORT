
#include "Octree.h"
#include "OctRT.h"
#include "colour.h"
#include "grid.h"

__kernel void DrawOctree(__write_only image2d_t output, __global Block* input, uint4 mipOffset, uint4 sizeAndLayer)
{
	int2 screenCoords = (int2)(get_global_id(0), get_global_id(1));
	uint2 dimensions = (uint2)(get_image_width(output), get_image_height(output));
		
	uint4 coords;

	coords.x = sizeAndLayer.x * ((float)screenCoords.x / (float)(dimensions.x));
	coords.y = sizeAndLayer.y * ((float)screenCoords.y / (float)(dimensions.y));
	coords.z = sizeAndLayer.w;
	
	uint4 size = sizeAndLayer;
	uint mipLevel = mipOffset.z % 16;	
	
	float4 colour;
	
	__global Block* current = input;
	uint curPos = 0;

	uint2 pixSize = dimensions;
		
	uint childOffset = 0;
	bool valid = true;
	for (int i = 0; i < mipLevel; ++i)
	{		
		pixSize /= 2;
		uint octant = getAndReduceOctant(&coords, &size);
		if (!getValid(current, octant))
		{	
			valid = false;
			i = 16;			
		}
	
		curPos += getChildPtr(current);
		curPos += octant;	
		current = input + curPos;
	}

	colour = UnpackColour(current->colour);
	
	//colour.x = 1.0;	
	
	if (!valid)
		colour.x = 1.0f;

	if ((screenCoords.x % pixSize.x) == 0 || (screenCoords.y % pixSize.y) == 0)
		colour.xyz = (float3)(1.0f, 1.0f, 1.0f);
	
	write_imagef(output, screenCoords, colour);
	
}