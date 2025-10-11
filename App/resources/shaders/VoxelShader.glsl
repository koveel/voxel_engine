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

layout(binding = 0) uniform sampler3D u_VoxelTexture;
layout(binding = 1) uniform sampler2D u_MaterialPalette;

uniform vec3 u_CameraPosition;

uniform int u_MaterialIndex;
uniform ivec3 u_VoxelDimensions;

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

void RaymarchVoxelMesh(
	vec3 rayOrigin, vec3 rayDirection, vec3 transformedObbCenter,
	out vec4 color, out vec3 normal, out float t, out ivec3 voxel
)
{
	const float VoxelScale = 0.1f;

	// Bounding box
	vec3 worldspaceExtents = (u_VoxelDimensions * VoxelScale) * 0.5f;
	vec3 p0 = transformedObbCenter - worldspaceExtents;
	vec3 p1 = transformedObbCenter + worldspaceExtents;

	float hit = RayAABB(rayOrigin, rayDirection, p0, p1);
	vec3 voxels_per_unit = u_VoxelDimensions / (p1 - p0);
	vec3 entry = ((rayOrigin + rayDirection * (hit + 0.0001f)) - p0) * voxels_per_unit;
	vec3 entry_worldspace = rayOrigin + rayDirection * hit;

	vec3 step = sign(rayDirection);
	vec3 delta = abs(1.0f / rayDirection);
	vec3 pos = clamp(floor(entry), vec3(0.0f), vec3(u_VoxelDimensions));
	vec3 tMax = (pos - entry + max(step, 0.0)) / rayDirection;

	int axis = 0;
	int maxSteps = u_VoxelDimensions.x + u_VoxelDimensions.y + u_VoxelDimensions.z;

	for (int i = 0; i < maxSteps; i++)
	{
		ivec3 voxelPos = ivec3(pos);

		color = texelFetch(u_VoxelTexture, voxelPos, 0);
		if (color.r != 0.0f)
		{
			voxel = voxelPos;

			// edge voxel
			if (i == 0)
			{
				t = hit;
				// Determine normal
				for (int a = 0; a < 3; a++)
				{
					float v = entry_worldspace[a];
					if (Approx(v, p0[a]) || Approx(v, p1[a]))
					{
						normal = vec3(0.0f);
						normal[a] = -step[a];
						break;
					}
				}
				return;
			}

			normal = vec3(0.0f);
			normal[axis] = -step[axis];

			t = hit + (tMax[axis] - delta[axis]) / voxels_per_unit[axis];
			return;
		}

		if (tMax.x < tMax.y)
		{
			if (tMax.x < tMax.z)
			{
				pos.x += step.x;
				if (pos.x < 0 || pos.x >= u_VoxelDimensions.x)
					break;
				tMax.x += delta.x;
				axis = 0;
			}
			else
			{
				pos.z += step.z;
				if (pos.z < 0 || pos.z >= u_VoxelDimensions.z)
					break;
				tMax.z += delta.z;
				axis = 2;
			}
		}
		else
		{
			if (tMax.y < tMax.z)
			{
				pos.y += step.y;
				if (pos.y < 0 || pos.y >= u_VoxelDimensions.y)
					break;
				tMax.y += delta.y;
				axis = 1;
			}
			else
			{
				pos.z += step.z;
				if (pos.z < 0 || pos.z >= u_VoxelDimensions.z)
					break;
				tMax.z += delta.z;
				axis = 2;
			}
		}
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

void main()
{
	vec3 cameraToPixel = normalize(u_CameraPosition - o_VertexWorldSpace);

	// Make ray relative to object orientation
	vec4 relativeCam = u_OBBOrientation * vec4(u_CameraPosition, 1.0f);
	// Normalized world space direction from camera to pixel
	vec4 relativeViewDir = u_OBBOrientation * vec4(cameraToPixel, 1.0f);
	vec3 transformedObbCenter = (u_OBBOrientation * vec4(u_OBBCenter, 1.0f)).xyz;

	vec4 colSample;
	vec3 normal;
	float t;
	ivec3 voxel;
	
	RaymarchVoxelMesh(relativeCam.xyz, -relativeViewDir.xyz, transformedObbCenter, colSample, normal, t, voxel);

	vec3 hitpoint = u_CameraPosition - cameraToPixel * t;
	gl_FragDepth = LinearizeDepth(hitpoint);

	int colorIndex = int(colSample.r * 255);
	o_Albedo = texelFetch(u_MaterialPalette, ivec2(colorIndex, u_MaterialIndex), 0);
	o_Normal = (u_OBBOrientation * vec4(normal, 1.0f)).xyz;
}