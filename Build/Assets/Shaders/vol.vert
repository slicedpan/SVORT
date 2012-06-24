#version 330

layout(location = 0) in vec3 position;

uniform sampler3D volTex;
uniform int sideLength;
uniform mat4 World; 
uniform mat4 viewProj;

smooth out vec4 colour;

void main()
{
	vec3 texCoord3D;
	ivec3 size = ivec3(sideLength, sideLength, sideLength);
	int num = gl_InstanceID;
	//int num = 1;
	texCoord3D.x = num % size.x;
	texCoord3D.y = (num / size.x) % size.y;
	texCoord3D.z = (num / (size.x * size.y));
	
	texCoord3D /= size;
	
	
	colour = texture(volTex, texCoord3D);
	//colour = vec4(texCoord3D, 1.0);
	//colour = vec4(1, 1, 1, 1);	
	
	vec4 vPos = vec4(vec3(texCoord3D + (position / size)), 0.1) * 10.0;
	
	gl_Position = viewProj *  vPos;
	
}