#version 450 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

//layout(r32f, binding = 0) uniform readonly image2D u_BlitFrom;
layout(binding = 0) uniform sampler2D u_BlitFrom;
layout(r32f, binding = 1) uniform writeonly image2D u_BlitTo;

void main()
{
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	vec2 viewport = vec2(imageSize(u_BlitTo));
	vec2 uv = (vec2(pixel) + 0.5f) / viewport;

	//float d = imageLoad(u_BlitFrom, pixel).r;
	float d = texture(u_BlitFrom, uv).r;
	imageStore(u_BlitTo, pixel, vec4(d));
}