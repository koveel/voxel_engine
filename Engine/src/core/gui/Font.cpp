#include "pch.h"

#include "Font.h"

#undef INFINITE
#include <msdf-atlas-gen.h>
#include <FontGeometry.h>
#include <GlyphGeometry.h>

#include "MSDFData.h"

namespace Engine {

	// Thanks Cherno!!
	template<typename T, typename S, int N, msdf_atlas::GeneratorFunction<S, N> GenFn>
	static owning_ptr<Texture2D> create_atlas(
		const std::filesystem::path& savePath,
		float fontSize, uint32_t atlasWidth, uint32_t atlasHeight,
		const std::vector<msdf_atlas::GlyphGeometry>& glyphs, const msdf_atlas::FontGeometry& geometry)
	{
		msdf_atlas::GeneratorAttributes generatorAttributes;
		generatorAttributes.scanlinePass = true;
		generatorAttributes.config.overlapSupport = true;

		msdf_atlas::ImmediateAtlasGenerator<S, N, GenFn, msdf_atlas::BitmapAtlasStorage<T, N>> generator(atlasWidth, atlasHeight);
		generator.setAttributes(generatorAttributes);
		generator.setThreadCount(8);
		generator.generate(glyphs.data(), (int)glyphs.size());

		msdfgen::BitmapConstRef<T, N> bitmap = (msdfgen::BitmapConstRef<T, N>)generator.atlasStorage();
		
		// cache atlas to file
		image_save_jpg_to_file(savePath, bitmap.width, bitmap.height, bitmap.pixels);
		auto atlas = Texture2D::create(bitmap.width, bitmap.height, TextureFormat::RGB8);
		atlas->set_wrap_mode(TextureWrapMode::Clamp);
		atlas->set_data(bitmap.pixels);

		return atlas;
	}

	Font::Font()
	{

	}

	owning_ptr<Font> Font::load_from_file(const std::filesystem::path& path)
	{
		auto result = owning_ptr<Font>(new Font());
		result->m_Data = new MSDFData();
		MSDFData* msdfData = result->m_Data;

		auto filename = path.filename();
		std::string fontName = filename.filename().string();
		std::string extension = path.extension().string();
		auto atlas_save_path = path.parent_path() / filename.replace_extension(".jpg");		

		msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
		ASSERT(ft && "failed to initialize freetype");

		std::string filepath = path.string();
		msdfgen::FontHandle* font = loadFont(ft, filepath.c_str());
		if (!font)
		{
			LOG("failed to load font {}", filepath.c_str());
			return nullptr;
		}

		static const std::pair<uint32_t, uint32_t> charRanges[] =
		{
			{ 0x0020, 0x00FF }, // latin + latin supp
		};

		msdf_atlas::Charset charset;
		for (auto& range : charRanges)
		{
			for (uint32_t c = range.first; c <= range.second; c++)
				charset.add(c);
		}
		
		double fontScale = 1.0;
		msdfData->FontGeometry = msdf_atlas::FontGeometry(&msdfData->Glyphs);
		msdfData->FontGeometry.setName(fontName.c_str());
		//msdfData->FontGeometry.setName(fontName.c_str());
		int glyphsLoaded = msdfData->FontGeometry.loadCharset(font, fontScale, charset);
		LOG("loaded {} glyphs out of {} from font {}", glyphsLoaded, charset.size(), fontName);

		double emSize = 80.0;
		msdf_atlas::TightAtlasPacker atlasPacker;
		atlasPacker.setPixelRange(2.0);
		atlasPacker.setMiterLimit(1.0);
		atlasPacker.setPadding(8);
		atlasPacker.setScale(emSize);
		int remaining = atlasPacker.pack(msdfData->Glyphs.data(), (int)msdfData->Glyphs.size());
		ASSERT(remaining == 0);

		int width, height;
		atlasPacker.getDimensions(width, height);
		emSize = atlasPacker.getScale();

		if (std::filesystem::exists(atlas_save_path)) {
			// load cached atlas
			result->m_AtlasTexture = Texture2D::load(atlas_save_path);
		}
		else {
#define THREAD_COUNT 8
#define DEFAULT_ANGLE_THRESHOLD 3.0
#define LCG_INCREMENT 1442695040888963407ull
#define LCG_MULTIPLIER 6364136223846793005ull

			// MSDF || MTSDF
			uint64_t coloringSeed = 0;
			bool expensiveColoring = true;
			if (expensiveColoring)
			{
				msdf_atlas::Workload([&glyphs = msdfData->Glyphs, &coloringSeed](int i, int threadNo) -> bool
				{
					unsigned long long glyphSeed = (LCG_MULTIPLIER * (coloringSeed ^ i) + LCG_INCREMENT) * !!coloringSeed;
					glyphs[i].edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
					return true;
				}, msdfData->Glyphs.size()).finish(THREAD_COUNT);
			}
			else
			{
				unsigned long long glyphSeed = coloringSeed;
				for (msdf_atlas::GlyphGeometry& glyph : msdfData->Glyphs)
				{
					glyphSeed *= LCG_MULTIPLIER;
					glyph.edgeColoring(msdfgen::edgeColoringInkTrap, DEFAULT_ANGLE_THRESHOLD, glyphSeed);
				}
			}

			result->m_AtlasTexture = create_atlas<uint8_t, float, 3, msdf_atlas::msdfGenerator>(atlas_save_path, (float)emSize, width, height, msdfData->Glyphs, msdfData->FontGeometry);
		}

		destroyFont(font);
		deinitializeFreetype(ft);

		return result;
	}

	Font::~Font()
	{
		delete m_Data;
	}

}