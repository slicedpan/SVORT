#version 330

layout(location = 0) in vec2 coord;
layout(location = 1) in vec2 texCoord;

smooth out vec2 fragTexCoord;

uniform vec2 pixSize;
uniform sampler3D baseTex;

uniform float zCoord;

void main()
{	
	fragTexCoord = texCoord;
	gl_Position = vec4(coord.x, coord.y, 0.0, 1.0);
} 
