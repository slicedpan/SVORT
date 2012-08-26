#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

#include "colour.h"
#include "grid.h"

__kernel void Mip(__global uint2* buffer, uint4 inSizeOffset, uint4 outSizeOffset, __global VoxelInfo* voxInfo)
{
	uint3 coords = (uint3)(get_global_id(0), get_global_id(1), get_global_id(2));
	uint inPos = inSizeOffset.w + GetGridOffset(coords * 2, inSizeOffset.xyz);
	uint outPos = outSizeOffset.w + GetGridOffset(coords, outSizeOffset.xyz);
	
	uint3 offsets = (uint3)(1, inSizeOffset.x, inSizeOffset.x * inSizeOffset.y);
	
	float4 colours[8];	
	float4 normals[8];

	colours[0] = UnpackColour(buffer[inPos].s0);	
	colours[1] = UnpackColour(buffer[inPos + offsets.x].s0);
	colours[2] = UnpackColour(buffer[inPos + offsets.x + offsets.y].s0);
	colours[3] = UnpackColour(buffer[inPos + offsets.x + offsets.z].s0);
	colours[4] = UnpackColour(buffer[inPos + offsets.x + offsets.y + offsets.z].s0);
	colours[5] = UnpackColour(buffer[inPos + offsets.y].s0);
	colours[6] = UnpackColour(buffer[inPos + offsets.y + offsets.z].s0);
	colours[7] = UnpackColour(buffer[inPos + offsets.z].s0);
	
	normals[0] = UnpackColour(buffer[inPos].s1);	
	normals[1] = UnpackColour(buffer[inPos + offsets.x].s1);
	normals[2] = UnpackColour(buffer[inPos + offsets.x + offsets.y].s1);
	normals[3] = UnpackColour(buffer[inPos + offsets.x + offsets.z].s1);
	normals[4] = UnpackColour(buffer[inPos + offsets.x + offsets.y + offsets.z].s1);
	normals[5] = UnpackColour(buffer[inPos + offsets.y].s1);
	normals[6] = UnpackColour(buffer[inPos + offsets.y + offsets.z].s1);
	normals[7] = UnpackColour(buffer[inPos + offsets.z].s1);	
	
	float4 totalColour = (float4)(0.0f, 0.0f, 0.0f, 0.0f);	
	float4 totalNormal = (float4)(0.0f, 0.0f, 0.0f, 0.0f);

	bool alpha = 0;
	int occupied = 0;

	for (int i = 0; i < 8; ++i)
	{
		totalColour += colours[i];
		if (colours[i].w > 0.0f)
		{
			totalNormal += normals[i];
			++occupied;
		}
	}		
	if (occupied)
	{
		totalColour /= occupied;
		totalNormal /= occupied;
		totalColour.w = 1.0f;
	}
	else
	{
		totalColour = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
		totalNormal = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
		atom_sub(&voxInfo->numLeafVoxels, 8);
		atom_sub(&voxInfo->pad, 8);
	}	
	
	buffer[outPos].s0 = PackColour(totalColour);
	buffer[outPos].s1 = PackColour(totalNormal);
	
}
