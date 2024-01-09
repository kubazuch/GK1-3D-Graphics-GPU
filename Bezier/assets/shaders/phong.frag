#version 330 core
				
layout(location = 0) out vec4 color;
				
in vec2 v_TexCoord;

struct material {
	sampler2D diffuse0;	
};

uniform material u_Material;

void main()
{
	color = texture(u_Material.diffuse0, v_TexCoord);
}
