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

layout(binding = 0) uniform sampler2D u_DepthTexture;
uniform vec2 u_ViewportDims;

uniform vec4 u_Color = vec4(1.0f);

float LinearizeDepth(float depth)
{
	const float near = 0.1f, far = 100.0f; 

	float ndc = depth * 2.0 - 1.0;
	return (2.0 * near * far) / (far + near - ndc * (far - near));
}

void main()
{
	vec2 uv = gl_FragCoord.xy / u_ViewportDims;
	float depth = texture(u_DepthTexture, uv).r;
	float d = gl_FragCoord.z;
	depth = LinearizeDepth(depth);
	d = LinearizeDepth(d);

	if (depth < d)
		discard;

	color = u_Color;
	color = vec4(u_Color.xyz, 0.5f);
}