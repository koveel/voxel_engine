#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transformation;

void main()
{
	gl_Position = u_ViewProjection * u_Transformation * vec4(a_Position, 1.0f);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 color;

void main()
{
	color = vec4(1.0f);
}