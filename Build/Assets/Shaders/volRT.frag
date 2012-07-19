#version 330

smooth in vec2 fragTexCoord;
uniform sampler3D baseTex;
uniform mat4 invWorldView;
uniform float maxDist;
uniform vec3 sphereCentre;
uniform ivec3 size;
uniform int maxIterations = 256;
out vec4 finalColour;
uniform float screenDist = 10.0;

struct ray
{
	vec3 origin;
	vec3 direction;
};

bool intersectSphere(ray r, float radius)
{
	vec3 endPoint = r.origin + (maxDist * r.direction);
	float dist = length(endPoint - r.origin);
	vec3 tmp1 = sphereCentre - r.origin;
	vec3 tmp2 = endPoint - r.origin;
	tmp1 *= tmp2;
	float u = tmp1.x + tmp1.y + tmp1.z;
	u /= dist * dist;
	
	if (u < 0.0  || u > 1.0)
		return false;
	vec3 intersection = r.origin + u * tmp2;
	if (length(sphereCentre - intersection) < radius)
		return true;
	return false;
}

float tmin, tmax, tymin, tymax, tzmin, tzmax;

vec3 intersectionPoint;

bool intersectCube(ray r, float t0, float t1)
{	
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
	intersectionPoint = r.origin + tmin * r.direction;
	return ( (tmin < t1) && (tmax > t0) );
}



void main()
{
	mat4 transform = invWorldView;
	ray r;
	r.origin = (transform * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
	r.direction = (transform * normalize(vec4(fragTexCoord.x, fragTexCoord.y, -screenDist, 0.0))).xyz;
	vec3 invSize = vec3(1.0 / size.x, 1.0 / size.y, 1.0 / size.z);	
	
	if (intersectCube(r, 1.0, 1000.0))
	{		
		vec3 offset = r.direction * length(invSize) * 0.5;
		ivec3 startPoint = ivec3(intersectionPoint * size + offset);
		bool hit = false;
		vec3 tDelta = abs(invSize / r.direction);		
		ivec3 cur = startPoint;
		ivec3 stepsize = ivec3(sign(r.direction));
		vec3 tMax = offset * stepsize;
		int iter = 0;
		ivec3 maxCoord = ivec3((size + (stepsize * size)) / 2.0);

		while(!hit && iter < maxIterations)
		{
			vec4 colour = texelFetch(baseTex, cur, 0);
			if (colour.a > 0.00001)
			{
				finalColour = colour;
				hit = true;
				break;
			}
			if(tMax.x < tMax.z) 
			{
				if(tMax.x < tMax.y) 
				{
					cur.x = cur.x + stepsize.x;
					if(cur.x == maxCoord.x)
						break;  // outside grid 
					tMax.x = tMax.x + tDelta.x;
				} 
				else 
				{
					cur.y = cur.y + stepsize.y;
					if(cur.y == maxCoord.y)
						break;
					tMax.y = tMax.y + tDelta.y;
				}
			} 
			else 
			{
				if(tMax.y < tMax.z) 
				{
				cur.y = cur.y + stepsize.y;
				if(cur.y == maxCoord.y)
					break;
				tMax.y = tMax.y + tDelta.y;
				}
				else 
				{
					cur.z = cur.z + stepsize.z;
					if(cur.z == maxCoord.z)
						break;
					tMax.z = tMax.z + tDelta.z;
				}
			}
			++iter;			
		} 	
		if (!hit)
			discard;
	}
	else
		discard;	
}