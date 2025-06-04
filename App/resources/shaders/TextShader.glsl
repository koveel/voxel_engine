#type vertex
#version 450 core

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_UV;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transformation;

out vec2 o_UV;

void main()
{
	o_UV = a_UV;
	gl_Position = u_ViewProjection * u_Transformation * vec4(a_Position, 0.0f, 1.0f);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 color;

in vec2 o_UV;

uniform sampler2D u_FontAtlas;

float median(vec3 v)
{
	return max(min(v.r, v.g), min(max(v.r, v.g), v.b));
}

const float pxRange = 2.0f;
float ScreenPxRange(vec2 uv)
{
	vec2 unitRange = vec2(pxRange, pxRange) / vec2(textureSize(u_FontAtlas, 0));
	vec2 screenTexSize = vec2(1.0f, 1.0f) / fwidth(uv);

	return max(0.5f * dot(unitRange, screenTexSize), 1.0f);
}

float SampleFontAtlas(vec2 uv)
{
	vec3 msd = texture2D(u_FontAtlas, uv).rgb;
	float screenPxDistance = ScreenPxRange(uv) * (median(msd) - 0.5f);
	float opacity = clamp(screenPxDistance + 0.5f, 0.0f, 1.0f);

	return opacity;
}

uniform vec4 u_ShadowColor = vec4(0.1f, 0.1f, 0.1f, 1.0f);
uniform vec2 u_ShadowOffset;

void main()
{
	const vec4 backgroundColor = vec4(0.0f);
	const vec4 textColor = vec4(1.0f);

	float baseOpacity = SampleFontAtlas(o_UV);
	float shadowOpacity = SampleFontAtlas(o_UV + u_ShadowOffset);

	vec4 foregroundColor = mix(backgroundColor, textColor, baseOpacity);
	color = foregroundColor;
}