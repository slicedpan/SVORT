#version 330

smooth in vec2 fragTexCoord;
uniform sampler3D baseTex;
uniform mat4 invWorld;
uniform mat4 invView;
uniform float maxDist;

out vec4 finalColour;

/*

bool intersectSphere(vec3 origin, vec3 dir, vec3 sphereCentre, float radius)
{
	vec3 dist = sphereCentre - origin;
	float B = dot(dir,dist);
	float D = B * B - length(dist) + radius * radius;
	if (D < 0.0)
		return false;
	return true;	
	float sqrtD = sqrt(D);
	float t0 = B - sqrtD;
	float t1 = B + sqrtD;
	if ((t0 > 0.01f) && t0 < 1000.0f)
		return true;
	if((t1 > 0.01f) && t1 < 1000.0f)
		return true;
	return false;	
	
}
*/

bool intersectSphere(vec3 origin, vec3 dir, vec3 sphereCentre, float radius)
{
	vec3 endPoint = origin + (maxDist * dir);
	float dist = length(endPoint - origin);
	vec3 tmp1 = sphereCentre - origin;
	vec3 tmp2 = endPoint - origin;
	tmp1 *= tmp2;
	float u = tmp1.x + tmp1.y + tmp1.z;
	u /= dist * dist;
	
	if (u < 0.0  || u > 1.0)
		return false;
	vec3 intersection = origin + u * tmp2;
	if (length(sphereCentre - intersection) < radius)
		return true;
	return false;
}

void main()
{
	mat4 transform = invView * invWorld;
	vec3 rayOrigin = (transform * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
	vec3 rayDir = (transform * normalize(vec4(fragTexCoord.x, fragTexCoord.y, -10.0, 0.0))).xyz;
	
	if (intersectSphere(rayOrigin, rayDir, vec3(0.0), 1.0f))
	{
		finalColour = vec4(1.0, 0.0, 0.0, 1.0);
	}
	else
		discard;	
}