#version 330

smooth in vec4 fragColour;

out vec4 finalColour;

void main()
{
	//if (colour.a <= 0.0001)
		//discard;
	finalColour = fragColour;
}

