// tessellation evaluation shader
#version 410 core

layout (quads, equal_spacing, ccw) in;

uniform mat4 u_M;
uniform mat4 u_VP;

// received from Tessellation Control Shader - all texture coordinates for the patch vertices
in vec2 TextureCoord[];

// send to Fragment Shader for coloring
out vec2 v_TexCoord;

uniform vec3 u_ControlPoints[16];

void main()
{
    // get patch coordinate
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    // ----------------------------------------------------------------------
    // retrieve control point texture coordinates
    vec2 t00 = TextureCoord[0];
    vec2 t01 = TextureCoord[1];
    vec2 t10 = TextureCoord[2];
    vec2 t11 = TextureCoord[3];

    // bilinearly interpolate texture coordinate across patch
    vec2 t0 = (t01 - t00) * u + t00;
    vec2 t1 = (t11 - t10) * u + t10;
    v_TexCoord = (t1 - t0) * v + t0;

    vec3 control_points[4];
    vec3 new_points[4];

    for(int j = 0; j < 4; ++j) {
        for(int i = 0; i < 4; ++i) {
             control_points[i] = u_ControlPoints[4*i + j];
        }   

        for(int k = 1; k < 4; ++k) {
            for(int i = 0; i < 4-k; ++i) {
                control_points[i] = (1 - v_TexCoord.x) * control_points[i] + v_TexCoord.x * control_points[i+1];
            }
        }

        new_points[j] = control_points[0];
    }

    for(int k = 1; k < 4; ++k) {
        for(int i = 0; i < 4-k; ++i) {
            new_points[i] = (1 - v_TexCoord.y) * new_points[i] + v_TexCoord.y * new_points[i + 1];
        }
    }

    // ----------------------------------------------------------------------
    // retrieve control point position coordinates
    vec4 p00 = gl_in[0].gl_Position;
    vec4 p01 = gl_in[1].gl_Position;
    vec4 p10 = gl_in[2].gl_Position;
    vec4 p11 = gl_in[3].gl_Position;

    // bilinearly interpolate position coordinate across patch
    vec4 p0 = (p01 - p00) * u + p00;
    vec4 p1 = (p11 - p10) * u + p10;
    vec4 p = (p1 - p0) * v + p0;

    // ----------------------------------------------------------------------
    // output patch point position in clip space
    gl_Position = u_VP * u_M * vec4(new_points[0], 1);
}