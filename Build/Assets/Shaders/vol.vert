#version 330

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;

uniform sampler3D volTex;
uniform ivec3 size;
uniform mat4 World; 
uniform mat4 View;
uniform mat4 Projection;

smooth out vec4 colour;


void main()
{
	vec3 texCoord3D;
	
	texCoord3D.x = gl_InstanceID % size.x;
	texCoord3D.y = (gl_InstanceID / size.x) % size.y;
	texCoord3D.z = (gl_InstanceID / (size.x * size.y));
	
	colour = texture(volTex, texCoord3D);
	
	texCoord3D /= size;	
	
	gl_Position = World * View * Projection * vec4(texCoord3D, 1.0) + vec4(position.x, position.y, 0.0, 0.0);
	
}