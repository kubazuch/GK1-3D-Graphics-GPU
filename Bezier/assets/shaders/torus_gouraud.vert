#version 410 core

layout (location = 0) in vec2 a_Texture;

out vec2 TexCoord;

void main()
{
	gl_Position = vec4(vec3(0.0), 1.0);
    TexCoord = a_Texture;
}