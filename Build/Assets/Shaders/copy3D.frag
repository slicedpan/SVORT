#version 330

out vec4 out_colour;
smooth in vec2 fragTexCoord;

uniform vec2 pixSize;
uniform sampler3D baseTex;

uniform float zCoord;

void main()
{	
	//out_colour = texture(baseTex, fragTexCoord);
	out_colour = texture(baseTex, vec3(fragTexCoord.x, fragTexCoord.y, zCoord));
}

