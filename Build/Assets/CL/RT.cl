
__constant const sampler_t sampler3D = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP;

#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

typedef struct _ray
{
	float4 origin;
	float4 direction;
} ray;

typedef struct 
{	
	float16 invWorldView;
	int4 size;
	float4 invSize;
} Params;

typedef struct
{
	int numSamples;
	int total;
} Counters;


float4 multVecMat(const float4* vector, __constant float16* matrix)
{
	float4 retVec;
	__constant float4* matPtr = (__constant float4*)matrix;
	retVec.x = dot(*matPtr, *vector);
	retVec.y = dot(*(matPtr + 1), *vector);
	retVec.z = dot(*(matPtr + 2), *vector);
	retVec.w = dot(*(matPtr + 3), *vector);
	
	return retVec;
}

float4 multMatVec(const float4* vector, __constant float16* matrix)
{
	float4 retVec;
	float4 tmp;
	tmp.x = (*matrix).s0;
	tmp.y = (*matrix).s4;
	tmp.z = (*matrix).s8;
	tmp.w = (*matrix).sC;
	retVec.x = dot(tmp, *vector);
	
	tmp.x = (*matrix).s1;
	tmp.y = (*matrix).s5;
	tmp.z = (*matrix).s9;
	tmp.w = (*matrix).sD;
	retVec.y = dot(tmp, *vector);

	tmp.x = (*matrix).s2;
	tmp.y = (*matrix).s6;
	tmp.z = (*matrix).sA;
	tmp.w = (*matrix).sE;
	retVec.z = dot(tmp, *vector);

	tmp.x = (*matrix).s3;
	tmp.y = (*matrix).s7;
	tmp.z = (*matrix).sB;
	tmp.w = (*matrix).sF;
	retVec.w = dot(tmp, *vector);
	
	return retVec;
}

bool intersectSphere(ray r, float radius, float4* intersectionPoint)
{
	float4 endPoint = r.origin + (1000.0 * r.direction);
	float dist = length(endPoint.xyz - r.origin.xyz);
	float4 tmp1 = - r.origin;
	float4 tmp2 = endPoint - r.origin;
	tmp1 *= tmp2;
	float u = tmp1.x + tmp1.y + tmp1.z;
	u /= dist * dist;
	
	if (u < 0.0  || u > 1.0)
		return false;
	*intersectionPoint = r.origin + u * tmp2;
	if (length((*intersectionPoint).xyz) < radius)
		return true;
	return false;
}

bool intersectCube(ray r, float t0, float t1, float4* intersectionPoint)
{
	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	if (r.direction.x >= 0) 
	{
		tmin = (- r.origin.x) / r.direction.x;
		tmax = (1 - r.origin.x) / r.direction.x;
	}
	else 
	{
		tmin = (1 - r.origin.x) / r.direction.x;
		tmax = (- r.origin.x) / r.direction.x;
	}
	if (r.direction.y >= 0) 
	{
		tymin = (- r.origin.y) / r.direction.y;
		tymax = (1 - r.origin.y) / r.direction.y;
	}
	else 
	{
		tymin = (1 - r.origin.y) / r.direction.y;
		tymax = (- r.origin.y) / r.direction.y;
	}	
	if ( (tmin > tymax) || (tymin > tmax) )
		return false;	

	if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;
	if (r.direction.z >= 0) 
	{
		tzmin = (- r.origin.z) / r.direction.z;
		tzmax = (1 - r.origin.z) / r.direction.z;
	}
	else 
	{
		tzmin = (1 - r.origin.z) / r.direction.z;
		tzmax = (- r.origin.z) / r.direction.z;
	}
	if ( (tmin > tzmax) || (tzmin > tmax) )
		return false;
	if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;
	(*intersectionPoint).xyz = r.origin.xyz + tmin * r.direction.xyz;
	return ( (tmin < t1) && (tmax > t0) );
}

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
	r.direction.xyz = normalize(r.direction.xyz);
	r.direction = multMatVec(&r.direction, &(params->invWorldView));
	
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
		maxCoord.x = (params->size.x + (stepSize.x * params->size.x)) / 2.0;
		maxCoord.y = (params->size.y + (stepSize.y * params->size.y)) / 2.0;
		maxCoord.z = (params->size.z + (stepSize.z * params->size.z)) / 2.0;
		
		bool hit = 0;
		
		int iter = 0;
		uint4 col;
		//float4 colour = params->invSize.xyzx * 128;
		//float4 colour = (float4)(1.0, 0.0, 0.0, 1.0);
		//float4 colour = (float4)(startPoint.x / 256.0, startPoint.y / 256.0, startPoint.z / 256.0, 1.0);		
		
		while(!hit && iter < 512)
		{
			int pos = startPoint.z * params->size.x * params->size.y + startPoint.y * params->size.x + startPoint.x;
			
			col.x = input[pos] & 0xff000000;
			col.y = input[pos] & 0x00ff0000;
			col.z = input[pos] & 0x0000ff00;
			col.w = input[pos] & 0x000000ff;
			if (col.w > 0)
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
			++iter;			
		}
		atom_add(&counters->total, iter);
		if (hit)
		{						
			write_imageui(bmp, coords, col);		
		}
	}	
	
}