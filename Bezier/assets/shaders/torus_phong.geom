#version 150
layout(triangles) in;

layout(line_strip, max_vertices=9) out;

uniform mat4 u_M;
uniform mat4 u_VP;

in vec3 v_Normal[];

out vec4 vertex_color;

void main()
{
  const float normal_length = 0.2;

  int i;
  for(i=0; i<gl_in.length(); i++)
  {
    vec3 P = gl_in[i].gl_Position.xyz;
    vec3 N = v_Normal[i];

    gl_Position = u_VP * u_M * vec4(P, 1.0);
    vertex_color = vec4(0,1,0,1);
    EmitVertex();
    
    gl_Position = u_VP * u_M * vec4(P + N * normal_length, 1.0);
    vertex_color = vec4(0,1,0,1);
    EmitVertex();

    gl_Position = u_VP * u_M * vec4(P, 1.0);
    vertex_color = vec4(1,0,0,1);
    EmitVertex();
    
    EndPrimitive();
  }
}