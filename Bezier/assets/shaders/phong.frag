#version 410 core

#include "material"
#include "light"

layout(location = 0) out vec4 color;
				
in vec3 v_Pos;
in vec2 v_TexCoord;
in vec3 v_Normal;

uniform material u_Material;
uniform point_light u_Light;
uniform vec3 u_Ambient;
uniform vec3 u_CameraPos;

void main()
{
	vec3 normal = normalize(v_Normal);
	vec4 object_color = texture(u_Material.diffuse[0], v_TexCoord);

	if(object_color.a < 0.5)
		discard;
	else
		object_color = vec4(object_color.rgb, 1.0);

	vec3 light = u_Material.ka * u_Ambient;
	
	light += calc_point_light(u_Light, u_Material, normal, v_Pos, normalize(u_CameraPos - v_Pos));
	color = vec4(light, 1.0) * object_color;
}
