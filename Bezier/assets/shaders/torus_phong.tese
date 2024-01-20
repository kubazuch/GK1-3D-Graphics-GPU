#version 410 core

layout (quads, equal_spacing, ccw) in;

uniform mat4 u_M;
uniform mat4 u_VP;

in vec2 TextureCoord[];

out vec3 v_Pos;
out vec2 v_TexCoord;
out vec3 v_Normal;

uniform float u_MinorRadius;

#define M_PI 3.1415926535897932384626433832795

void main()
{
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec2 t00 = TextureCoord[0];
    vec2 t01 = TextureCoord[1];
    vec2 t10 = TextureCoord[2];
    vec2 t11 = TextureCoord[3];

    vec2 t0 = (t01 - t00) * u + t00;
    vec2 t1 = (t11 - t10) * u + t10;
    v_TexCoord = (t1 - t0) * v + t0;
    
    float r = 0.1;
    float phi = 2 * (1 - v_TexCoord.x) * M_PI;
    float theta =  2 * (1 - v_TexCoord.y) * M_PI;

    vec3 center = vec3(cos(phi), 0, sin(phi));
    vec3 normal = vec3(-u_MinorRadius * cos(theta) * cos(phi), u_MinorRadius * sin(theta), -u_MinorRadius * cos(theta) * sin(phi));
    vec3 pos = center + normal;
    
    v_Normal = normalize((inverse(transpose(u_M)) * vec4(normal, 0)).xyz);

    gl_Position = u_VP * u_M * vec4(pos, 1);
    v_Pos = (u_M * vec4(pos, 1)).xyz;
    //gl_Position = vec4(pos, 1);
}