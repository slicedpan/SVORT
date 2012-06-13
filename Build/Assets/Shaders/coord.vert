#version 330

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 inColour;

uniform mat4 wvp;

smooth out vec4 fragColour;

void main()
{	
	fragColour = inColour;
	gl_Position = wvp * position;
}