#version 330

smooth in vec4 colour;
out vec4 outColour;

void main()
{
	if (colour.a <= 0.00001)
		discard;
	outColour = colour;
}

