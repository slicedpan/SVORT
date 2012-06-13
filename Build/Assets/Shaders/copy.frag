#version 330

out vec4 out_colour;
smooth in vec2 fragTexCoord;

uniform vec2 pixSize;
uniform sampler2D baseTex;

void main()
{	
	//out_colour = texture(baseTex, fragTexCoord);
	out_colour = texture(baseTex, fragTexCoord);
}

