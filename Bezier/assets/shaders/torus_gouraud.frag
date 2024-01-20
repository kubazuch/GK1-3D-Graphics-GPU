#version 410 core

#include "material"
#include "light"

in vec3 v_Pos;
in vec2 v_TexCoord;
in vec3 v_Normal;
in vec3 v_Light;

in vec4 vertex_color;

layout(location = 0) out vec4 color;

uniform material u_Material;
uniform vec3 u_Ambient;
uniform vec3 u_CameraPos;

uniform float u_Fog;

void main()
{
	vec4 object_color = texture(u_Material.diffuse[0], v_TexCoord);

	if(object_color.a < 0.5)
		discard;
	else
		object_color = vec4(object_color.rgb, 1.0);

	if(u_Material.emissive) {
		color = object_color;
	} else {
		color = vec4(v_Light, 1.0) * object_color;
	}

	float factor = length(u_CameraPos - v_Pos) * u_Fog;
	float alpha = 1 / exp(factor * factor);
	color = mix(vec4(u_Ambient, 1.0), color, alpha);
}