#version 330

layout(location = 0) in vec3 position;

uniform sampler3D volTex;
uniform int sideLength;
uniform mat4 World; 
uniform mat4 viewProj;

uniform ivec3 size;

smooth out vec4 colour;

const vec3 halfVec = vec3(0.5, 0.5, 0.5);

void main()
{
	ivec3 texCoord3D;	
	int num = gl_InstanceID;
	//int num = 1;
	texCoord3D.x = num % size.x;
	texCoord3D.y = (num / size.x) % size.y;
	texCoord3D.z = (num / (size.x * size.y));
	
	colour = texelFetch(volTex, texCoord3D, 0);
	//colour = vec4(texCoord3D, 1.0);
	//colour = vec4(1, 1, 1, 1);	
	
	vec4 vPos = vec4(((texCoord3D + position) / size), 0.1) * 10.0;
	
	gl_Position = viewProj *  vPos;
	
}