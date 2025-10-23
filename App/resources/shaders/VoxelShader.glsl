#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transformation;

out vec3 o_VertexWorldSpace;

void main()
{
	vec4 transformed = u_Transformation * vec4(a_Position, 1.0f);
	o_VertexWorldSpace = transformed.xyz;
	gl_Position = u_ViewProjection * transformed;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Albedo;
layout(location = 1) out vec3 o_Normal;

in vec3 o_VertexWorldSpace;

layout(binding = 0) uniform usampler3D u_VoxelTexture;
layout(binding = 1) uniform sampler2D u_MaterialPalette;
layout(binding = 2) uniform sampler2D u_Texture;

uniform int u_TextureTileFactor = 1;

uniform vec3 u_CameraPosition;

uniform int u_MaterialIndex;
uniform int u_MipLevel = 0;

uniform vec3 u_OBBCenter;
uniform mat4 u_OBBOrientation;

float RayAABB(vec3 ro, vec3 rd, vec3 p0, vec3 p1)
{
	float tmin = 0, tmax = 1e30f;

	for (int axis = 0; axis < 3; axis++)
	{
		float t1 = (p0[axis] - ro[axis]) / rd[axis];
		float t2 = (p1[axis] - ro[axis]) / rd[axis];

		float dmin = min(t1, t2);
		float dmax = max(t1, t2);

		tmin = max(dmin, tmin);
		tmax = min(dmax, tmax);
	}

	return tmax >= tmin ? tmin : 1e30f;
}

bool Approx(float a, float b)
{
	const float epsilon = 0.00001f;
	return abs(a - b) < epsilon;
}

vec2 ComputeFaceUV(vec3 local, vec3 normal)
{
	vec2 uv;
	if (abs(normal.x) > 0.5f)
		uv = local.zy;
	else if (abs(normal.y) > 0.5f)
		uv = local.xz;
	else
		uv = local.xy;

	uv.x = 1.0f - uv.x;

	return uv;
}

void RaymarchVoxelMesh(
	vec3 rayOrigin, vec3 rayDirection, vec3 obbCenter,
	out int color, out vec3 normal, out float t, out ivec3 voxel, out vec2 uv
)
{
	const float BaseVoxelScale = 0.1f;
	float voxelScale = BaseVoxelScale * exp2(u_MipLevel);

	// Bounding box
	ivec3 mipDimensions = textureSize(u_VoxelTexture, u_MipLevel);
	vec3 worldspaceExtents = (vec3(mipDimensions) * voxelScale) * 0.5f;
	vec3 p0 = obbCenter - worldspaceExtents;
	vec3 p1 = obbCenter + worldspaceExtents;

	float hit = RayAABB(rayOrigin, rayDirection, p0, p1);
	vec3 voxelsPerUnit = mipDimensions / (p1 - p0);
	vec3 entry = ((rayOrigin + rayDirection * (hit + 0.0001f)) - p0) * voxelsPerUnit;
	vec3 entryWorldspace = rayOrigin + rayDirection * hit;

	ivec3 step = ivec3(sign(rayDirection));
	vec3 delta = abs(1.0f / rayDirection);
	ivec3 pos = ivec3(clamp(floor(entry), vec3(0.0f), vec3(mipDimensions - 1)));
	vec3 tMax = (vec3(pos) - entry + max(vec3(step), 0.0)) / rayDirection;

	int axis = 0;
	int maxSteps = mipDimensions.x + mipDimensions.y + mipDimensions.z;

	for (int i = 0; i < maxSteps; i++)
	{
		uint col = texelFetch(u_VoxelTexture, pos, u_MipLevel).r;
		if (col != 0)
		{
			color = int(col);
			voxel = pos;			

			// edge voxel
			if (i == 0)
			{
				t = hit;
				// Determine normal
				for (int a = 0; a < 3; a++)
				{
					float v = entryWorldspace[a];
					if (Approx(v, p0[a]) || Approx(v, p1[a]))
					{
						normal = vec3(0.0f);
						normal[a] = -step[a];
						break;
					}
				}

				// uv
				vec3 gridPos = (entryWorldspace - p0) * voxelsPerUnit;
				vec3 local = fract(gridPos / u_TextureTileFactor);
				uv = ComputeFaceUV(local, normal);

				return;
			}

			normal = vec3(0.0f);
			normal[axis] = -float(step[axis]);
			t = hit + (tMax[axis] - delta[axis]) / voxelsPerUnit[axis];
			
			// uv
			vec3 worldHit = rayOrigin + rayDirection * t;
			vec3 gridPos = (worldHit - p0) * voxelsPerUnit;
			vec3 local = fract(gridPos / u_TextureTileFactor);
			uv = ComputeFaceUV(local, normal);

			return;
		}

		if (tMax.x < tMax.y) {
			if (tMax.x < tMax.z) {
				pos.x += step.x;
				tMax.x += delta.x;
				axis = 0;
			}
			else {
				pos.z += step.z;
				tMax.z += delta.z;
				axis = 2;
			}
		}
		else {
			if (tMax.y < tMax.z) {
				pos.y += step.y;
				tMax.y += delta.y;
				axis = 1;
			}
			else {
				pos.z += step.z;
				tMax.z += delta.z;
				axis = 2;
			}
		}

		if (any(lessThan(pos, ivec3(0))) || any(greaterThanEqual(pos, mipDimensions)))
			break;
	}

	discard;
}

uniform mat4 u_ViewProjection;

float LinearizeDepth(vec3 p)
{
	vec4 clipPos = u_ViewProjection * vec4(p, 1.0f);
	vec3 ndc = clipPos.xyz / clipPos.w;
	return (ndc.z + 1.0f) / 2.0f;
}

void GetVoxelTangentBasis(vec3 normal, out vec3 T, out vec3 B)
{
	if (normal.x > 0.5) {          // +X face
		T = vec3(0, 0, 1);
		B = vec3(0, -1, 0);
	}
	else if (normal.x < -0.5) {    // -X face
		T = vec3(0, 0, -1);
		B = vec3(0, -1, 0);
	}
	else if (normal.y > 0.5) {     // +Y face
		T = vec3(1, 0, 0);
		B = vec3(0, 0, 1);
	}
	else if (normal.y < -0.5) {    // -Y face
		T = vec3(1, 0, 0);
		B = vec3(0, 0, -1);
	}
	else if (normal.z > 0.5) {     // +Z face
		T = vec3(1, 0, 0);
		B = vec3(0, -1, 0);
	}
	else {                         // -Z face
		T = vec3(-1, 0, 0);
		B = vec3(0, -1, 0);
	}
}

void main()
{
	vec3 cameraToPixel = normalize(u_CameraPosition - o_VertexWorldSpace);

	// Make ray relative to object orientation
	vec4 relativeCam = u_OBBOrientation * vec4(u_CameraPosition, 1.0f);
	// Normalized world space direction from camera to pixel
	vec4 relativeViewDir = u_OBBOrientation * vec4(cameraToPixel, 1.0f);
	vec3 transformedObbCenter = (u_OBBOrientation * vec4(u_OBBCenter, 1.0f)).xyz;

	int colorIndex;
	vec3 normal;
	float t;
	ivec3 voxel;
	vec2 uv;
	
	// march
	RaymarchVoxelMesh(relativeCam.xyz, -relativeViewDir.xyz, transformedObbCenter, colorIndex, normal, t, voxel, uv);

	// hitpoint / depth
	vec3 hitpoint = u_CameraPosition - cameraToPixel * t;
	float depth = LinearizeDepth(hitpoint);
	gl_FragDepth = depth;

	// normals
	const float NormalMapStrength = 0.5f;
	normal = (u_OBBOrientation * vec4(normal, 1.0f)).xyz;
	vec3 normal_map = (u_OBBOrientation * vec4(texture(u_Texture, uv).rgb, 1.0f)).xyz;
	o_Normal = mix(normal, normal_map, NormalMapStrength);

	vec3 T, B;
	GetVoxelTangentBasis(normal, T, B);

	vec3 tangentNormal = texture(u_Texture, uv).xyz * 2.0 - 1.0;
	vec3 worldNormal = normalize(
		tangentNormal.x * T +
		tangentNormal.y * B +
		tangentNormal.z * normal
	);

	o_Normal = normal;

	vec4 albedo = texelFetch(u_MaterialPalette, ivec2(colorIndex, u_MaterialIndex), 0);
	o_Albedo = albedo;
}