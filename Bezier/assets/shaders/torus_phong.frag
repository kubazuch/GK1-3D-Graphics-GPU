#version 410 core

#include "material"
#include "light"

in vec3 v_Pos;
in vec2 v_TexCoord;
in vec3 v_Normal;

in vec4 vertex_color;

layout(location = 0) out vec4 color;

uniform material u_Material;
uniform vec3 u_Ambient;
uniform vec3 u_CameraPos;

uniform int u_PointCount;
uniform point_light u_PointLights[MAX_LIGHTS];
uniform int u_DirectionalCount;
uniform directional_light u_DirectionalLights[MAX_LIGHTS];
uniform int u_SpotCount;
uniform spot_light u_SpotLights[MAX_LIGHTS];

uniform float u_Fog;

void main()
{
	vec3 normal = normalize(v_Normal);
	vec4 object_color = texture(u_Material.diffuse[0], v_TexCoord);

	if(object_color.a < 0.5)
		discard;
	else
		object_color = vec4(object_color.rgb, 1.0);

	if(u_Material.emissive) {
		color = object_color;
	} else {
		vec3 light = u_Material.ka * u_Ambient;
	
		for(int i = 0; i < u_PointCount; ++i)
			light += calc_point_light(u_PointLights[i], u_Material, normal, v_Pos, normalize(u_CameraPos - v_Pos));
	
		for(int i = 0; i < u_SpotCount; ++i)
			light += calc_spot_light(u_SpotLights[i], u_Material, normal, v_Pos, normalize(u_CameraPos - v_Pos));
	
		color = vec4(light, 1.0) * object_color;
	}

	float factor = length(u_CameraPos - v_Pos) * u_Fog;
	float alpha = 1 / exp(factor * factor);
	color = mix(vec4(u_Ambient, 1.0), color, alpha);
}