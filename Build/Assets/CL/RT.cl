
__constant const sampler_t sampler3D = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP;

#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#include "colour.h"
#include "RT.h"
#include "GenRay.h"
#include "RayIntersect.h"

__kernel void VolRT(__write_only image2d_t bmp, __global int* input, __constant Params* params, __global Counters* counters)
{

	int x = get_global_id(0);
	int y = get_global_id(1);
	int w = get_global_size(0) - 1;
	int h = get_global_size(1) - 1;
	
	float xPos = (float)x / (float)w;
	float yPos = (float)y / (float)h;	

	float2 screenPos = (float2)((float)x / (float)w, (float)y / (float)h);

	Ray r = createRay(&params->invWorldView, screenPos);	
	
	int2 coords = (int2)(x, y);	

	float4 intersectionPoint = (float4)(0.0, 0.0, 0.0, 1.0);

	atom_add(&counters->numSamples, 1);	

	if (intersectCube(r, 0.001, 1000.0, &intersectionPoint))
	{				
		int4 startPoint;
		float3 stepSize = sign(r.direction.xyz);
		float3 stepFlag = stepSize * 0.5 + 0.5;	//1 if positive, 0 otherwise
		int3 maxCoord;

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

		float3 tDelta = fabs(1.0 / r.direction.xyz);

		float3 tMax;
		tMax.x = (startPoint.x + stepFlag.x) - intersectionPoint.x;	
		tMax.y = (startPoint.y + stepFlag.y) - intersectionPoint.y;
		tMax.z = (startPoint.z + stepFlag.z) - intersectionPoint.z;

		tMax /= r.direction.xyz;

		maxCoord.x = (params->size.x + (stepSize.x * params->size.x)) / 2 + stepSize.x * 0.5 - 0.5;	//branchless, set to minus one if stepSize is negative
		maxCoord.y = (params->size.y + (stepSize.y * params->size.y)) / 2 + stepSize.y * 0.5 - 0.5;
		maxCoord.z = (params->size.z + (stepSize.z * params->size.z)) / 2 + stepSize.z * 0.5 - 0.5;
		
		bool hit = 0;
		hit = 0;
		
		int iter = 0;
		float4 colour;		
		//colour = params->invSize.xyzx * 8;
		//colour = (float4)(1.0, 0.0, 0.0, 1.0);
		//colour = (float4)((float)startPoint.x / (params->size.x - 1), (float)startPoint.y / (params->size.y - 1), (float)startPoint.z / (params->size.z - 1), 1.0);	
		//colour = (float4)((float)maxCoord.x / params->size.x, (float)maxCoord.y / params->size.y, (float)maxCoord.z / params->size.z, 1.0);

		/*float4 clamped = clamp(colour, 0.0, 1.0);
		if (colour.x != clamped.x || colour.y != clamped.y || colour.z != clamped.z)
			colour = (float4)(1.0, 0.0, 0.0, 1.0);*/

		/*if (maxCoord.x > params->size.x - 1 || maxCoord.y > params->size.y - 1 || maxCoord.z > params->size.x - 1)
			colour = (float4)(1.0, 0.0, 0.0, 1.0);*/

		if (startPoint.x == 0 && startPoint.y == 0 && startPoint.z == 0)
		{
			colour = (float4)(1.0, 0.0, 0.0, 1.0);		
			hit = 1;	
		}
		
		while(!hit && iter < 512)
		{
			int pos = startPoint.z * params->size.x * params->size.y + startPoint.y * params->size.x + startPoint.x;
			
			colour = UnpackColour(input[pos]);
			
			++iter;
			
			if (colour.w > 0.0)
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
