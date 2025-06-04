#pragma once

#include "rendering/Texture.h"

namespace Engine {

	struct MSDFData;

	class Font
	{
	private:
		Font();
	public:
		~Font();

		MSDFData* get_msdf_data() const { return m_Data; }
		Texture2D* get_atlas() const { return m_AtlasTexture.get(); }

		static std::unique_ptr<Font> load_from_file(const std::filesystem::path& file);
	private:
		MSDFData* m_Data = nullptr;
		std::unique_ptr<Texture2D> m_AtlasTexture = nullptr;
	};

}