#version 410 core

#include "material"
#include "light"

layout (quads, equal_spacing, ccw) in;

uniform mat4 u_M;
uniform mat4 u_VP;

in vec2 TextureCoord[];

out vec3 v_Pos;
out vec2 v_TexCoord;
out vec3 v_Normal;
out vec3 v_Light;

uniform float u_MinorRadius;

uniform material u_Material;
uniform vec3 u_Ambient;
uniform vec3 u_CameraPos;

uniform int u_PointCount;
uniform point_light u_PointLights[MAX_LIGHTS];
uniform int u_DirectionalCount;
uniform directional_light u_DirectionalLights[MAX_LIGHTS];
uniform int u_SpotCount;
uniform spot_light u_SpotLights[MAX_LIGHTS];

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

    v_Light = u_Material.ka * u_Ambient;
	
	for(int i = 0; i < u_PointCount; ++i)
		v_Light += calc_point_light(u_PointLights[i], u_Material, v_Normal, v_Pos, normalize(u_CameraPos - v_Pos));
	
	for(int i = 0; i < u_SpotCount; ++i)
		v_Light += calc_spot_light(u_SpotLights[i], u_Material, v_Normal, v_Pos, normalize(u_CameraPos - v_Pos));
}