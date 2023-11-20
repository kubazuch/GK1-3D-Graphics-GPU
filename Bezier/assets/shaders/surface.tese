// tessellation evaluation shader
#version 410 core

layout (quads, equal_spacing, ccw) in;

uniform mat4 u_M;
uniform mat4 u_VP;
uniform bool u_Geometry;

// received from Tessellation Control Shader - all texture coordinates for the patch vertices
in vec2 TextureCoord[];

// send to Fragment Shader for coloring
out vec3 v_Pos;
out vec2 v_TexCoord;
out vec3 v_Normal;
out mat3 v_TBN;

const int N = 4;
uniform vec3 u_ControlPoints[N * N];

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

    
    // ----------------------------------------------------------------------
    // de Casteljau

    vec3 intermediate_point[N];
    vec3 point[N];

    vec3 intermediate_tangent_x[N-1];
    vec3 tangent_x[N];

    vec3 intermediate_tangent_y[N];
    vec3 tangent_y[N-1];

    for(int j = 0; j < N; ++j) {
        for(int i = 0; i < N; ++i) {
            intermediate_point[i] = u_ControlPoints[i * N + j];
            if(i < N-1)
                intermediate_tangent_x[i] = u_ControlPoints[(i + 1) * N + j] - u_ControlPoints[i * N + j];
            if(j < N-1)
                intermediate_tangent_y[i] = u_ControlPoints[i * N + j + 1] - u_ControlPoints[i * N + j];
        }

        for(int k = 1; k < N; ++k) {
            for(int i = 0; i < N-k; ++i) {
                intermediate_point[i] = (1 - v_TexCoord.x) * intermediate_point[i] + v_TexCoord.x * intermediate_point[i+1];
                intermediate_tangent_y[i] = (1 - v_TexCoord.x) * intermediate_tangent_y[i] + v_TexCoord.x * intermediate_tangent_y[i+1];
                if(i < N-1-k)
                    intermediate_tangent_x[i] = (1 - v_TexCoord.x) * intermediate_tangent_x[i] + v_TexCoord.x * intermediate_tangent_x[i+1];
            }
        }

        point[j] = intermediate_point[0];
        tangent_x[j] = intermediate_tangent_x[0];
        if(j < N-1)
            tangent_y[j] = intermediate_tangent_y[0];
    }

    for(int k = 1; k < N; ++k) {
        for(int i = 0; i < N-k; ++i) {
            point[i] = (1 - v_TexCoord.y) * point[i+1] + v_TexCoord.y * point[i];
            tangent_x[i] = (1 - v_TexCoord.y) * tangent_x[i+1] + v_TexCoord.y * tangent_x[i];
            if(i < N-1-k)
                tangent_y[i] = (1 - v_TexCoord.y) * tangent_y[i+1] + v_TexCoord.y * tangent_y[i];
        }
    }

    // ----------------------------------------------------------------------
    // output patch point position in clip space
    if(u_Geometry)
        gl_Position = vec4(point[0], 1);
    else {
        gl_Position = u_VP * u_M * vec4(point[0], 1);
        v_Pos = (u_M * vec4(point[0], 1)).xyz; 
    }
    vec3 tangent = normalize(vec3(u_M * vec4(tangent_x[0], 0.0)));
    vec3 bitangent = -normalize(vec3(u_M * vec4(tangent_y[0], 0.0)));
    v_Normal = normalize(cross(tangent, bitangent));
    v_TBN = mat3(tangent, bitangent, v_Normal);
}