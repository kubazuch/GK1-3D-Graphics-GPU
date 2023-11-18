// vertex shader
#version 410 core

// vertex position
layout (location = 0) in int a_Index;
// texture coordinate
layout (location = 1) in vec2 a_Texture;

out vec2 TexCoord;

uniform vec3 u_ControlPoints[16];

void main()
{
    // convert XYZ vertex to XYZW homogeneous coordinate
	gl_Position = vec4(u_ControlPoints[a_Index], 1.0);
    // pass texture coordinate though
    TexCoord = a_Texture;
}