#type vertex
#version 450 core

layout(location = 0) in vec2 a_Position;

void main()
{
	gl_Position = vec4(a_Position, 0.0f, 1.0f);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

layout(binding = 0) uniform sampler2D u_Albedo;
layout(binding = 1) uniform sampler2D u_Normals;
layout(binding = 2) uniform sampler2D u_AO;
layout(binding = 3) uniform sampler2D u_Lighting;
layout(binding = 4) uniform sampler2D u_ComputeShit;

uniform vec2 u_ViewportDims;

uniform int u_Output;

void main()
{
	vec2 uv = gl_FragCoord.xy / u_ViewportDims;

	//vec4 albedo = texture(u_Albedo, uv);
	//float ao = texture(u_AO, uv).r;
	//vec3 normal = texture(u_Normals, uv).xyz;
	//vec4 lighting = texture(u_Lighting, uv);
	//
	//vec4 final = albedo * vec4(vec3(ao), 1.0f) + lighting;
	//switch (u_Output)
	//{
	//case 0: o_Color = final; break;
	//case 1: o_Color = albedo; break;
	//case 2: o_Color = vec4(normal, 1.0f); break;
	//case 3: o_Color = vec4(0.0f, 0.0f, ao, 1.0f);
	//}
	
	o_Color = texture(u_ComputeShit, uv);
}