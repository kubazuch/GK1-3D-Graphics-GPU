// tessellation evaluation shader
#version 410 core

layout (quads, equal_spacing, ccw) in;

uniform mat4 u_M;
uniform mat4 u_VP;
uniform bool u_Geometry;

// received from Tessellation Control Shader - all texture coordinates for the patch vertices
in vec2 TextureCoord[];

// send to Fragment Shader for coloring
out vec3 v_Pos;
out vec2 v_TexCoord;
out vec3 v_Normal;
out mat3 v_TBN;

uniform bool u_UseHeight = false;
uniform sampler2D u_HeightTexture;
uniform float u_HeightMod = 1.0/50.0;

#define M_PI 3.1415926535897932384626433832795

void main()
{
    // get patch coordinate
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    // ----------------------------------------------------------------------
    // retrieve control point texture coordinates
    vec2 t00 = TextureCoord[0];
    vec2 t01 = TextureCoord[1];
    vec2 t10 = TextureCoord[2];
    vec2 t11 = TextureCoord[3];

    // bilinearly interpolate texture coordinate across patch
    vec2 t0 = (t01 - t00) * u + t00;
    vec2 t1 = (t11 - t10) * u + t10;
    v_TexCoord = (t1 - t0) * v + t0;
    
    // ----------------------------------------------------------------------
    // Sphere
    float r = 0.1;
    float phi = -(2 * v_TexCoord.x - 1) * M_PI;
    float theta = (v_TexCoord.y - 0.5) * M_PI;

    vec3 pos = vec3(cos(theta) * cos(phi), sin(theta), cos(theta)*sin(phi));
    
    // ----------------------------------------------------------------------
    // output patch point position in clip space
    v_Normal = normalize((u_M * vec4(pos, 0)).xyz);

	if(u_UseHeight){
        float height = texture(u_HeightTexture, v_TexCoord).r * u_HeightMod;
        pos += v_Normal * height;
    }
    gl_Position = u_VP * u_M * vec4(pos, 1);
    v_Pos = (u_M * vec4(pos, 1)).xyz; 
    vec3 bitangent = cross(v_Normal, vec3(0,1,0));
    vec3 tangent = cross(bitangent, v_Normal);
    v_TBN = mat3(tangent, bitangent, v_Normal);
}