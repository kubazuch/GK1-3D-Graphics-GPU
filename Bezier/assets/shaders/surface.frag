#version 410 core

in vec2 v_TexCoord;
in vec3 v_Normal;
in vec4 vertex_color;
uniform sampler2D u_Texture;

layout(location = 0) out vec4 color;

void main()
{
	color = texture(u_Texture, v_TexCoord) + vertex_color;
	//color = vec4(v_TexCoord, 0.0, 1.0);
}