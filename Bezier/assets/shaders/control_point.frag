#version 330 core
				
layout(location = 0) out vec4 color;
layout(location = 1) out int color2;
				
in vec2 v_TexCoord;

uniform vec3 u_Color;
uniform int u_Id;

void main()
{
	color = vec4(u_Color, 1.0);
	color2 = u_Id;
}
