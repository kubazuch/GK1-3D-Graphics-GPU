// tessellation control shader
#version 410 core

// specify number of control points per patch output
// this value controls the size of the input and output arrays
layout (vertices=4) out;

uniform int u_HDensity;
uniform int u_VDensity;

// varying input from vertex shader
in vec2 TexCoord[];
// varying output to evaluation shader
out vec2 TextureCoord[];

void main()
{
    // ----------------------------------------------------------------------
    // pass attributes through
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    TextureCoord[gl_InvocationID] = TexCoord[gl_InvocationID];

    // ----------------------------------------------------------------------
    // invocation zero controls tessellation levels for the entire patch
    if (gl_InvocationID == 0)
    {
        gl_TessLevelOuter[0] = u_VDensity;
        gl_TessLevelOuter[1] = u_HDensity;
        gl_TessLevelOuter[2] = u_VDensity;
        gl_TessLevelOuter[3] = u_HDensity;

        gl_TessLevelInner[0] = u_HDensity;
        gl_TessLevelInner[1] = u_VDensity;
    }
}
