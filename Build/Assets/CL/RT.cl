
__constant const sampler_t sampler3D = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP;

#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#include "colour.h"
#include "RT.h"
#include "GenRay.h"
#include "RayIntersect.h"

__kernel void VolRT(__write_only image2d_t bmp, __global uint2* input, __constant Params* params, __global Counters* counters)
{

	int x = get_global_id(0);
	int y = get_global_id(1);
	int w = get_global_size(0) - 1;
	int h = get_global_size(1) - 1;
	
	float xPos = (float)x / (float)w;
	float yPos = (float)y / (float)h;
	
	int2 coords = (int2)(x, y);		

	if (x == (w + 1) / 2 && y == (h + 1) / 2)
	{
		write_imagef(bmp, coords, (float4)(1.0, 0.0, 0.0, 1.0));
		return;
	}

	float2 screenPos = (float2)((float)x / (float)w, (float)y / (float)h);

	Ray r = createRay(&params->invWorldView, screenPos);	
	
	

	float4 intersectionPoint = params->camPos;	

	if (intersectionPoint.s3 > 0.0f || intersectCube(r, 0.001f, 1000.0f, &intersectionPoint))
	{				

		atom_add(&counters->numSamples, 1);	

		uint3 startPoint;
		int3 stepSize;

		stepSize.x = sign(r.direction.x);
		stepSize.y = sign(r.direction.y);
		stepSize.z = sign(r.direction.z);


		intersectionPoint.x *= params->size.x;
		intersectionPoint.y *= params->size.y;
		intersectionPoint.z *= params->size.z;
		
		startPoint.x = floor(intersectionPoint.x);
		startPoint.y = floor(intersectionPoint.y);
		startPoint.z = floor(intersectionPoint.z);	//start point is integer position in voxel grid

		//if startPoint coords are equal to size, then subtract 1 to keep it inside the grid
		if (startPoint.x == params->size.x) --startPoint.x;
		if (startPoint.y == params->size.y) --startPoint.y;
		if (startPoint.z == params->size.z) --startPoint.z;

		float3 tDelta = fabs(1.0f / r.direction.xyz);

		float3 tMax;
		tMax.x = startPoint.x - intersectionPoint.x + (stepSize.x + 1) / 2.0f;	
		tMax.y = startPoint.y - intersectionPoint.y + (stepSize.y + 1) / 2.0f;
		tMax.z = startPoint.z - intersectionPoint.z + (stepSize.z + 1) / 2.0f;

		tMax /= r.direction.xyz;
		
		bool hit = 0;
		//hit = 1;
		
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
		int pos = 0;

		while(!hit && iter < 512)
		{
			pos = startPoint.z * params->size.x * params->size.y + startPoint.y * params->size.x + startPoint.x;
			
			colour = UnpackColour(input[pos].s0);
			
			++iter;
			
			if (colour.w > 0.0f)
			{				
				hit = true;
				break;
			}
			if(tMax.x < tMax.z) 
			{
				if(tMax.x < tMax.y) 
				{
					startPoint.x = startPoint.x + stepSize.x;
					if(startPoint.x >= params->size.x)
						break;  // outside grid 
					tMax.x = tMax.x + tDelta.x;					
				} 
				else 
				{
					startPoint.y = startPoint.y + stepSize.y;
					if(startPoint.y >= params->size.y)
						break;
					tMax.y = tMax.y + tDelta.y;
				}
			} 
			else 
			{
				if(tMax.y < tMax.z) 
				{
					startPoint.y = startPoint.y + stepSize.y;
					if(startPoint.y >= params->size.y)
						break;
					tMax.y = tMax.y + tDelta.y;
				}
				else 
				{
					startPoint.z = startPoint.z + stepSize.z;
					if(startPoint.z >= params->size.z)
						break;
					tMax.z = tMax.z + tDelta.z;
				}
			}		
		}
		atom_add(&counters->total, iter);
		if (hit)
		{		
			float4 normal = UnpackColour(input[pos].s1);
			float4 lightColour = (float4)(1.0f, 0.6f, 0.2f, 1.0f);
			float4 worldPos = (float4)(startPoint.x * params->invSize.x, startPoint.y * params->invSize.y, startPoint.z * params->invSize.z, 1.0f);
			float lightAmount = max(dot(normal.xyz, normalize(params->lightPos.xyz - worldPos.xyz)), 0.3f);
			colour *= lightColour * lightAmount;				
			write_imagef(bmp, coords, colour);		
		}
	}	
}
