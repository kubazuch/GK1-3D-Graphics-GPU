#version 410 core

#include "material"
#include "light"

in vec3 v_Pos;
in vec2 v_TexCoord;
in vec3 v_Normal;
in mat3 v_TBN;

in vec4 vertex_color;

uniform sampler2D u_Texture;
uniform sampler2D u_NormalTexture;
uniform sampler2D u_AOTexture;
uniform bool u_UseTexture = false;
uniform bool u_UseAO = false;
uniform bool u_InvertNormalY = false;
uniform int u_NormalMode;

layout(location = 0) out vec4 color;
layout(location = 1) out int color2;

uniform material u_Material;
uniform point_light u_Light;
uniform vec3 u_Ambient;
uniform vec3 u_CameraPos;
uniform int u_Id;

void main()
{
	vec3 object_color;
	if(u_UseTexture)
		object_color = texture(u_Texture, v_TexCoord).rgb;
	else
		object_color = u_Material.color;

	vec3 normal = v_Normal;
	if(u_NormalMode != 0) {
		normal = texture(u_NormalTexture, v_TexCoord).rgb;
		normal = normalize(normal * 2.0 - 1.0);

		if(u_InvertNormalY)
			normal.y = -normal.y;

		normal = normalize(v_TBN * normal);
	}
	
	if(u_NormalMode == 2) {
		normal = normalize(normal + v_Normal);
	}

	vec3 result = u_Material.ka * u_Ambient;
	if(u_UseAO)
		result *= texture(u_AOTexture, v_TexCoord).r;
	result += calc_point_light(u_Light, u_Material, normal, v_Pos, normalize(u_CameraPos - v_Pos));
	color = vec4(result * object_color, 1.0);
	color2 = u_Id;
}