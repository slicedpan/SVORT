#version 330

layout(location = 0) in vec2 coord;

void main()
{		
	gl_Position = vec4(coord.x, coord.y, 0.0, 1.0);
} 
