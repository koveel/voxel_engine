#version 450 core

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(rgba8, binding = 0) uniform writeonly image2D o_Image;

void main()
{
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	ivec2 viewport = imageSize(o_Image);
    uvec2 group = gl_WorkGroupID.xy;
    uvec2 local = gl_LocalInvocationID.xy;

    vec4 color = vec4(vec2(pixel) / vec2(viewport), 0.0f, 1.0f);
    imageStore(o_Image, pixel, color);
}