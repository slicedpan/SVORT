#version 330

vec3 lightPos = vec3(0, 4, 0);
float lightRadius = 20;

layout(location = 0)out vec4 out_colour;
smooth in vec3 oNormal;
smooth in vec3 worldPos;

uniform vec3 lightCone;

void main()
{	

	/*
	if (abs(worldPos.z) > 1.1)
	{
		discard;
	}
	//*/
	vec3 lightDir = lightPos - worldPos;
	float lightDist = length(lightDir);
	lightDir /= lightDist;
	vec3 lightColour;
	float coneAtten = pow(max(dot(lightCone, lightDir), 0.0), 0.1);	
	lightDir = vec3(0, 1, 0);
	float nDotL = max(0.0, dot(lightDir, normalize(oNormal)));		
    
	float attenuation = pow(max((1 - lightDist/lightRadius), 0.0), 1.4) * coneAtten;
	lightColour = vec3(1.0, 0.7, 0.6) * nDotL;// * attenuation;
	vec3 diffuse = vec3(0.2, 0.2, 0.2);
	lightColour = max(lightColour, diffuse);

	out_colour = vec4(lightColour, 0.33333);
	
}

