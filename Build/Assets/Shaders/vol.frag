#version 330

smooth in vec4 colour;

void main()
{
	//if (colour.a <= 0.0001)
		//discard;
	gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}

