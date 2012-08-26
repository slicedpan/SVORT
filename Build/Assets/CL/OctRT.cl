
__constant const sampler_t sampler3D = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP;

#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#define PERFCOUNTERENABLED
#include "colour.h"
#define STACKSWITCH
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

	float4 intersectionPoint = params->camPos;	

	if (intersectionPoint.s3 > 0.0f || intersectCube(r, 0.001f, 1000.0f, &intersectionPoint))
	{		
		r.origin = intersectionPoint;
		atom_add(&counters->numSamples, 1);	

		float4 colour;
		
		uint curPos = 0;	
		__global Block* current;	
		float t;
		curPos = rayTrace2(input, &r, params->size, counters, 32, &t);
		if (curPos & HITORMISSBIT)
			return;
		if (curPos > 2480)
			return;
		current = input + (curPos & POSMASK);
		colour = UnpackColour(current->colour);	

		float4 normal = UnpackColour(current->normal);
		float4 lightColour = (float4)(1.0f, 0.6f, 0.2f, 1.0f);
		float4 worldPos = r.origin + r.direction * t;
		float lightAmount = max(dot(normal.xyz, normalize(params->lightPos.xyz - worldPos.xyz)), 0.3f);
		colour *= lightColour * lightAmount;				
				
		write_imagef(bmp, coords, colour);	
	}
}