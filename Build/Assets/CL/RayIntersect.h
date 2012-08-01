
bool intersectSphere(Ray r, float radius, float4* intersectionPoint)
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

bool intersectCube(Ray r, float t0, float t1, float4* intersectionPoint)
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