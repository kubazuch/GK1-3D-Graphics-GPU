#version 410 core

#include "material"
#include "light"

in vec3 v_Pos;
in vec2 v_TexCoord;
flat in vec3 v_Normal;
flat in vec4 v_Color;

in vec4 vertex_color;

layout(location = 0) out vec4 color;

uniform material u_Material;
uniform vec3 u_Ambient;
uniform vec3 u_CameraPos;

uniform float u_Fog;

void main()
{
	color = v_Color;

	if(v_Color.a < 0.5)
		discard;
	else
		color = vec4(v_Color.rgb, 1.0);

	float factor = length(u_CameraPos - v_Pos) * u_Fog;
	float alpha = 1 / exp(factor * factor);
	color = mix(vec4(u_Ambient, 1.0), color, alpha);
}