#version 330 core

#include "material"
#include "light"
				
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
				
uniform mat4 u_M;
uniform mat4 u_VP;

out vec3 v_Pos;
out vec2 v_TexCoord;
out vec3 v_Normal;
out vec3 v_Light;

uniform material u_Material;
uniform vec3 u_Ambient;
uniform vec3 u_CameraPos;

uniform int u_PointCount;
uniform point_light u_PointLights[MAX_LIGHTS];
uniform int u_DirectionalCount;
uniform directional_light u_DirectionalLights[MAX_LIGHTS];
uniform int u_SpotCount;
uniform spot_light u_SpotLights[MAX_LIGHTS];

void main() {
	v_Pos = (u_M * vec4(a_Position, 1.0)).xyz;
	v_TexCoord = a_TexCoord;
	v_Normal = normalize((inverse(transpose(u_M)) * vec4(a_Normal, 0.0)).xyz);
	gl_Position = u_VP * u_M * vec4(a_Position, 1.0);

	
	v_Light = u_Material.ka * u_Ambient;
	
	for(int i = 0; i < u_PointCount; ++i)
		v_Light += calc_point_light(u_PointLights[i], u_Material, v_Normal, v_Pos, normalize(u_CameraPos - v_Pos));
	
	for(int i = 0; i < u_SpotCount; ++i)
		v_Light += calc_spot_light(u_SpotLights[i], u_Material, v_Normal, v_Pos, normalize(u_CameraPos - v_Pos));
}