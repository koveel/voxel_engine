#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transformation;

out vec3 o_Vertex;

void main()
{
	vec4 transformed = u_Transformation * vec4(a_Position, 1.0f);
	o_Vertex = transformed.xyz;
	gl_Position = u_ViewProjection * transformed;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 color;

in vec3 o_Vertex;

uniform vec3 u_Color;
uniform vec3 u_VolumeCenter;
uniform float u_Radius;

uniform float AttConst;
uniform float AttLinear;
uniform float AttExp;

uniform vec2 u_ViewportDims;

#include "raytracing.glinc"

void main()
{
	vec2 uv = gl_FragCoord.xy / u_ViewportDims;

	vec3 worldPos = ReconstructWorldSpaceFromDepth(uv);

	vec3 lightDir = worldPos - u_VolumeCenter;

	float dist = length(lightDir);
	lightDir = normalize(lightDir);
	
	float att = AttConst + (AttLinear * dist) + AttExp * (dist * dist);

	color = att * vec4(u_Color, 1.0f);
}