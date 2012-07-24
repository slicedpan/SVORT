#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

#include "colour.h"
#include "grid.h"

__kernel void Mip(__global int* buffer, int4 inSizeOffset, int4 outSizeOffset)
{
	int3 coords = (int3)(get_global_id(0), get_global_id(1), get_global_id(2));
	int inPos = inSizeOffset.w + GetGridOffset(coords * 2, inSizeOffset.xyz);
	int outPos = outSizeOffset.w + GetGridOffset(coords, outSizeOffset.xyz);
	
	int3 offsets = (int3)(1, inSizeOffset.x, inSizeOffset.x * inSizeOffset.y);
	
	float4 colours[8];
	colours[0] = UnpackColour(buffer[inPos]);
	colours[1] = UnpackColour(buffer[inPos + offsets.x]);
	colours[2] = UnpackColour(buffer[inPos + offsets.x + offsets.y]);
	colours[3] = UnpackColour(buffer[inPos + offsets.x + offsets.z]);
	colours[4] = UnpackColour(buffer[inPos + offsets.x + offsets.y + offsets.z]);
	colours[5] = UnpackColour(buffer[inPos + offsets.y]);
	colours[6] = UnpackColour(buffer[inPos + offsets.y + offsets.z]);
	colours[7] = UnpackColour(buffer[inPos + offsets.z]);	
	
	float4 totalColour = (float4)(0.0, 0.0, 0.0, 0.0);	
	
	bool alpha = 0;

	for (int i = 0; i < 8; ++i)
	{
		totalColour += colours[i];
		if (colours[i].w > 0.0 && !alpha)
			alpha = 1;
	}		
	
	totalColour /= 8.0;
	
	if (alpha)
		totalColour.w = 1.0;
	
	buffer[outPos] = PackColour(totalColour);
	
}
