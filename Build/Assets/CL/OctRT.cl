
__constant const sampler_t sampler3D = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP;

#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#define PERFCOUNTERENABLED
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

	if (intersectCube(r, 0.001, 1000.0, &intersectionPoint))
	{
		uint4 startPoint;

		intersectionPoint.x *= params->size.x;
		intersectionPoint.y *= params->size.y;
		intersectionPoint.z *= params->size.z;
		
		startPoint.x = max(floor(intersectionPoint.x), 0.0f);
		startPoint.y = max(floor(intersectionPoint.y), 0.0f);
		startPoint.z = max(floor(intersectionPoint.z), 0.0f);	//start point is integer position in voxel grid

		//if startPoint coords are equal to size, then subtract 1 to keep it inside the grid
		if (startPoint.x == params->size.x) --startPoint.x;
		if (startPoint.y == params->size.y) --startPoint.y;
		if (startPoint.z == params->size.z) --startPoint.z;		

		float4 colour;

		VoxelStack vs;
		initStack(&vs);
		uint curPos = findStartPoint(startPoint, input, &vs, params->size, counters, 32);
		__global Block* current;

		if (curPos & HITORMISSBIT)	//we've hit a leaf already
		{
			current = input + (curPos & POSMASK);
			colour = UnpackColour(current->colour);			
		}	
		else
		{	
			curPos = castRay(startPoint, intersectionPoint, input, &vs, &r, params->size, counters, 32);
			if (curPos & HITORMISSBIT)
				return;
			current = input + (curPos & POSMASK);
			colour = UnpackColour(current->colour);			
		}	
		write_imagef(bmp, coords, colour);	
	}
}