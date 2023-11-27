// vertex shader
#version 410 core

// vertex position
layout (location = 0) in vec2 a_Texture;

out vec2 TexCoord;

void main()
{
    // convert XYZ vertex to XYZW homogeneous coordinate
	gl_Position = vec4(vec3(0.0), 1.0);
    // pass texture coordinate though
    TexCoord = a_Texture;
}