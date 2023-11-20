#version 410 core

in vec2 v_TexCoord;
uniform sampler2D u_Texture;

layout(location = 0) out vec4 color;
uniform vec3 u_Color;

void main()
{
	color = vec4(u_Color, 1);
}