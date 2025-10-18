#type vertex
#version 450 core

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_UV;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transformation;

out vec2 v_UV;

void main()
{
	v_UV = a_UV;
	gl_Position = u_ViewProjection * u_Transformation * vec4(a_Position, 0.0f, 1.0f);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D u_Texture;

uniform vec4 u_Tint = vec4(1.0f);

in vec2 v_UV;

void main()
{
	vec4 tex = texture(u_Texture, v_UV);
	color = u_Tint * tex;
}