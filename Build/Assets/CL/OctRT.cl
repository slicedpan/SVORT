
__constant const sampler_t sampler3D = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP;

#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#include "colour.h"
#include "OctRT.h"
#include "GenRay.h"
#include "RayIntersect.h"


__kernel void OctRT(__write_only image2d_t bmp, __global Block* input, __constant Params* params, __global Counters* counters)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	int w = get_global_size(0) - 1;
	int h = get_global_size(1) - 1;
	
	float2 screenPos = (float2)((float)x / (float)w, (float)y / (float)h);

	Ray r = createRay(&params->invWorldView, screenPos);	
	
	int2 coords = (int2)(x, y);	

	float4 intersectionPoint = (float4)(0.0, 0.0, 0.0, 1.0);

	atom_add(&counters->numSamples, 1);	

	if (intersectCube(r, 1.0, 1000.0, &intersectionPoint))
	{
		float4 offset = r.direction.xyz * length(params->invSize.xyz) * 0.5;  //move halfway into voxel		
		float4 startPoint;
		uint3 startVoxel;
		
		startPoint.x = intersectionPoint.x * params->size.x + offset.x;
		startPoint.y = intersectionPoint.y * params->size.y + offset.y;
		startPoint.z = intersectionPoint.z * params->size.z + offset.z;	//start point is integer position in voxel grid

		offset = fract(startPoint, &intersectionPoint);

		startVoxel.s0 = intersectionPoint.s0;
		startVoxel.s1 = intersectionPoint.s1;
		startVoxel.s2 = intersectionPoint.s2;		

		float4 colour;

		VoxelStack vs;
		initStack(&vs);
		uint curPos = findStartPoint(&startVoxel, input, &vs, params, counters, 32);	
		__global Block* current = input + curPos;
		colour = UnpackColour(current->colour);
		if (colour.w == 0.0)
			colour = (float4)(1.0, 0.0, (curPos % 255) / 255.0, 1.0);	
		write_imagef(bmp, coords, colour);
	}
}