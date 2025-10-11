#type vertex
#version 450 core

layout(location = 0) in vec2 a_Position;

void main()
{
	gl_Position = vec4(a_Position, 0.0f, 1.0f);
}

#type fragment
#version 450 core

layout(location = 2) out vec3 o_OldNormal;
layout(location = 5) out float o_OldDepth;

layout(binding = 0) uniform sampler2D u_DepthBuffer;
layout(binding = 1) uniform sampler2D u_NormalBuffer;

uniform vec2 u_ViewportDims;

void main()
{
	vec2 uv = gl_FragCoord.xy / u_ViewportDims;

	float depth = texture(u_DepthBuffer, uv).r;
	vec3 normal = texture(u_NormalBuffer, uv).xyz;

	o_OldDepth = depth;
	o_OldNormal = normal;
}