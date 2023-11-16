#version 330 core
				
layout(location = 0) out vec4 color;
layout(location = 1) out int color2;
				
in vec2 v_TexCoord;

uniform sampler2D u_Texture;

uniform vec4 u_Color;
uniform int u_Id;

void main()
{
	color = u_Color; //texture(u_Texture, v_TexCoord);
	color2 = u_Id;
}
