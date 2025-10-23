#version 450 core

layout(rgba8, binding = 0) uniform writeonly image2D u_OutputImage;

layout(binding = 0) uniform sampler2D u_Albedo;
layout(binding = 1) uniform sampler2D u_Normals;
layout(binding = 2) uniform sampler2D u_AO;
//layout(binding = 3) uniform sampler2D u_Lighting;

void main()
{
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	ivec2 viewport = imageSize(u_Output);
	vec2 uv = vec2(pixel) / vec2(viewport);

	vec4 albedo = texture(u_Albedo, uv);
	float ao = texture(u_AO, uv).r;
	vec3 normal = texture(u_Normals, uv).xyz;
	vec4 lighting = texture(u_Lighting, uv);
	
	vec4 final = albedo * vec4(vec3(ao), 1.0f) + lighting;
	o_Color = final;
	o_Color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	//switch (u_Output)
	//{
	//case 0: o_Color = final; break;
	//case 1: o_Color = albedo; break;
	//case 2: o_Color = vec4(normal, 1.0f); break;
	//case 3: o_Color = vec4(0.0f, 0.0f, ao, 1.0f);
	//}
}