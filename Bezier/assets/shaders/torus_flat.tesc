#version 410 core

layout (vertices=4) out;

uniform int u_HDensity;
uniform int u_VDensity;

in vec2 TexCoord[];
out vec2 TextureCoord[];

void main()
{
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    TextureCoord[gl_InvocationID] = TexCoord[gl_InvocationID];

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
