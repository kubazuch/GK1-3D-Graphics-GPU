#version 410 core

in vec3 v_Pos;
in vec2 v_TexCoord;
in vec3 v_Normal;
in mat3 v_TBN;

in vec4 vertex_color;

uniform sampler2D u_Texture;
uniform sampler2D u_NormalTexture;
uniform bool u_UseTexture;
uniform bool u_Geometry;
uniform int u_NormalMode;

layout(location = 0) out vec4 color;

struct material {
	float ka;
	float kd;
	float ks;
	float m;

	vec3 color;
};

struct light {
	vec3 pos;
	vec3 color;
};

uniform material u_Material;
uniform light u_Light;
uniform vec3 u_Ambient;
uniform vec3 u_CameraPos;

void main()
{
	if(u_Geometry) {
		color = vertex_color;
		return;
	}

	vec3 object_color;
	if(u_UseTexture)
		object_color = texture(u_Texture, v_TexCoord).rgb;
	else
		object_color = u_Material.color;

	vec3 normal = v_Normal;
	if(u_NormalMode != 0) {
		normal = texture(u_NormalTexture, v_TexCoord).rgb;
		normal = normalize(normal * 2.0 - 1.0);

		normal = normalize(v_TBN * normal);
	}
	
	if(u_NormalMode == 2) {
		normal = normalize(normal + v_Normal);
	}

	vec3 ambient = u_Material.ka * u_Ambient;

	vec3 light_dir = normalize(u_Light.pos - v_Pos);
	float diff = max(dot(normal, light_dir), 0.0);
	vec3 diffuse = u_Material.kd * diff * u_Light.color;

	vec3 view_dir = normalize(u_CameraPos - v_Pos);
	vec3 reflect_dir = reflect(-light_dir, normal);
	float spec = pow(max(dot(view_dir, reflect_dir), 0.0), u_Material.m);
	vec3 specular = u_Material.ks * spec * u_Light.color;  
	
	vec3 result = (ambient + diffuse + specular) * object_color;
	color = vec4(result, 1.0);
}