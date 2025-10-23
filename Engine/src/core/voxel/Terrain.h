#pragma once

namespace Engine {

	class Terrain
	{
	public:
		struct GenerationParameters
		{
			struct
			{
				float amplitude = 0.8f;
				float frequency = 0.8f;
				float lacunarity = 0.6f;
				float persistence = 0.8f;
				uint32_t octaves = 3;
				float scale = 1.0f / 30.0f;
			} noise;

			uint32_t width = 0; // x and z
			uint32_t height = 0;
		};
	
		static VoxelMesh generate_terrain(const GenerationParameters& params);
	};

} 