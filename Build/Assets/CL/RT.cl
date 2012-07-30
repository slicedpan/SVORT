
__constant const sampler_t sampler3D = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP;

#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#include "colour.h"
#include "RT.h"

__kernel void VolRT(__write_only image2d_t bmp, __global int* input, __constant Params* params, __global Counters* counters)
{

	int x = get_global_id(0);
	int y = get_global_id(1);
	int w = get_global_size(0) - 1;
	int h = get_global_size(1) - 1;
	
	float xPos = (float)x / (float)w;
	float yPos = (float)y / (float)h;	

	ray r;
	r.origin = (float4)(0.0, 0.0, 0.0, 1.0);
	r.origin = multMatVec(&r.origin, &(params->invWorldView));
	r.direction = (float4)((xPos * 2.0) - 1.0, (yPos * 2.0) - 1.0, -1.0, 0.0);	
	r.direction = multMatVec(&r.direction, &(params->invWorldView));
	r.direction.xyz = normalize(r.direction.xyz);
	
	int2 coords = (int2)(x, y);	

	float4 intersectionPoint = (float4)(0.0, 0.0, 0.0, 1.0);

	atom_add(&counters->numSamples, 1);	

	if (intersectCube(r, 1.0, 1000.0, &intersectionPoint))
	{		
		float3 offset = r.direction.xyz * length(params->invSize.xyz) * 0.5;  //move halfway into voxel		
		int4 startPoint;
		
		startPoint.x = intersectionPoint.x * params->size.x + offset.x;
		startPoint.y = intersectionPoint.y * params->size.y + offset.y;
		startPoint.z = intersectionPoint.z * params->size.z + offset.z;	//start point is integer position in voxel grid
		

		float3 tDelta = fabs(params->invSize.xyz / r.direction.xyz);		
		float3 stepSize = sign(r.direction.xyz);
		float3 tMax = offset * stepSize;
		
		int3 maxCoord;
		maxCoord.x = (params->size.x + (stepSize.x * params->size.x)) / 2.0 - 1;
		maxCoord.y = (params->size.y + (stepSize.y * params->size.y)) / 2.0 - 1;
		maxCoord.z = (params->size.z + (stepSize.z * params->size.z)) / 2.0 - 1;
		
		bool hit = 0;
		//hit = 1;
		
		int iter = 0;
		float4 colour;		
		//colour = params->invSize.xyzx * 128;
		//colour = (float4)(1.0, 0.0, 0.0, 1.0);
		//colour = (float4)(startPoint.x / 128.0, startPoint.y / 128.0, startPoint.z / 128.0, 1.0);			
		
		while(!hit && iter < 512)
		{
			int pos = startPoint.z * params->size.x * params->size.y + startPoint.y * params->size.x + startPoint.x;
			
			colour = UnpackColour(input[pos]);
			
			++iter;
			
			if (colour.w > 0)
			{				
				hit = true;
				break;
			}
			if(tMax.x < tMax.z) 
			{
				if(tMax.x < tMax.y) 
				{
					startPoint.x = startPoint.x + stepSize.x;
					if(startPoint.x == maxCoord.x)
						break;  // outside grid 
					tMax.x = tMax.x + tDelta.x;
				} 
				else 
				{
					startPoint.y = startPoint.y + stepSize.y;
					if(startPoint.y == maxCoord.y)
						break;
					tMax.y = tMax.y + tDelta.y;
				}
			} 
			else 
			{
				if(tMax.y < tMax.z) 
				{
				startPoint.y = startPoint.y + stepSize.y;
				if(startPoint.y == maxCoord.y)
					break;
				tMax.y = tMax.y + tDelta.y;
				}
				else 
				{
					startPoint.z = startPoint.z + stepSize.z;
					if(startPoint.z == maxCoord.z)
						break;
					tMax.z = tMax.z + tDelta.z;
				}
			}		
		}
		atom_add(&counters->total, iter);
		if (hit)
		{						
			write_imagef(bmp, coords, colour);		
		}
	}	
}

__kernel void OctRT(__write_only image2d_t bmp, __global Block* input, __constant Params* params, __global Counters* counters)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	int w = get_global_size(0) - 1;
	int h = get_global_size(1) - 1;
	
	float xPos = (float)x / (float)w;
	float yPos = (float)y / (float)h;	

	ray r;
	r.origin = (float4)(0.0, 0.0, 0.0, 1.0);
	r.origin = multMatVec(&r.origin, &(params->invWorldView));
	r.direction = (float4)((xPos * 2.0) - 1.0, (yPos * 2.0) - 1.0, -1.0, 0.0);	
	r.direction = multMatVec(&r.direction, &(params->invWorldView));
	r.direction.xyz = normalize(r.direction.xyz);
	
	int2 coords = (int2)(x, y);	

	float4 intersectionPoint = (float4)(0.0, 0.0, 0.0, 1.0);

	atom_add(&counters->numSamples, 1);	

	if (intersectCube(r, 1.0, 1000.0, &intersectionPoint))
	{
		float3 offset = r.direction.xyz * length(params->invSize.xyz) * 0.5;  //move halfway into voxel		
		uint3 startPoint;
		uint3 size = params->size.xyz;
		
		startPoint.x = intersectionPoint.x * params->size.x + offset.x;
		startPoint.y = intersectionPoint.y * params->size.y + offset.y;
		startPoint.z = intersectionPoint.z * params->size.z + offset.z;	//start point is integer position in voxel grid

		float4 colour = (float4)(1.0, 0.0, 0.0, 1.0);

		int iter = 0;
		bool hit = 0;
		uint curPos = 0;
		__global Block* current = 0;

		VoxelStack vs;
		initStack(&vs);

		while (iter < 512 && !hit)
		{	
			uint octant = getOctant(startPoint, size);
			if (current)
			{
				if (!getValid(current, octant))
				{	
					hit = true;
				}
			}
			curPos += octant;
			current = input + curPos;		
			uint childOffset = getChildPtr(current);
			if (!childOffset)
			{
				hit = true;
				break;
			}
			curPos += childOffset;	
			reduceOctant(&startPoint, &size);
		}
		atom_add(&counters->total, iter);
		colour = UnpackColour(current->colour);
		if (hit)
		{
			write_imagef(bmp, coords, colour);
		}

	}
}