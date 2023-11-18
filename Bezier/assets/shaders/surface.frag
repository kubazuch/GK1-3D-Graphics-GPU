#version 410 core

in vec2 v_TexCoord;
uniform sampler2D u_Texture;

layout(location = 0) out vec4 color;

void main()
{
	color = texture(u_Texture, v_TexCoord);
	//color = vec4(v_TexCoord, 0.0, 1.0);
}