#include "pch.h"

#include "Font.h"
#include "MSDFData.h"

#include "rendering/Shader.h"
#include "rendering/VertexArray.h"
#include "rendering/Graphics.h"
#include "rendering/Texture.h"

namespace Engine {

	struct TextVertex
	{
		Float2 position;
		Float2 texCoord;
	};

	static constexpr size_t MaxQuadsPerBatch    = 8000;
	static constexpr size_t MaxVerticesPerBatch = MaxQuadsPerBatch * 4;
	static constexpr size_t MaxIndicesPerBatch  = MaxQuadsPerBatch * 6;

	static TextVertex* s_VertexData = nullptr;
	static TextVertex* s_CurrentVertex = nullptr;

	static std::unique_ptr<VertexArray> s_TextVAO;
	static VertexBuffer* s_TextVBO;

	void init_text()
	{
		s_TextVAO = VertexArray::create();

		s_VertexData = new TextVertex[MaxVerticesPerBatch];
		s_CurrentVertex = s_VertexData;

		uint32_t* indices = new uint32_t[MaxIndicesPerBatch];
		uint32_t offset = 0;
		for (size_t i = 0; i < MaxIndicesPerBatch; i += 6)
		{
			indices[i + 0] = offset + 0;
			indices[i + 1] = offset + 1;
			indices[i + 2] = offset + 2;
			indices[i + 3] = offset + 2;
			indices[i + 4] = offset + 3;
			indices[i + 5] = offset + 0;
			offset += 4;
		}
		s_TextVAO->set_index_buffer(IndexBuffer::create(indices, MaxIndicesPerBatch));
		delete[] indices;

		auto vbo = VertexBuffer::create(nullptr, sizeof(TextVertex) * MaxVerticesPerBatch);
		vbo->set_layout({
			{ ShaderDataType::Float2 },
			{ ShaderDataType::Float2 },
		});

		s_TextVBO = vbo.get();
		s_TextVAO->add_vertex_buffer(std::move(vbo));

		//s_TextShader = Shader::create("resources/shaders/TextShader.glsl");
	}

	void Graphics::draw_text(const std::string& text, const std::unique_ptr<class Font>& pFont, float tracking)
	{
		Font& font = *pFont;
		
		float lineHeight = 1.0f;
		const msdf_atlas::FontGeometry& fontGeometry = font.get_msdf_data()->FontGeometry;
		const msdfgen::FontMetrics& metrics = fontGeometry.getMetrics();
		Texture2D* atlas = font.get_atlas();

		double x = 0.0;
		double y = 0.0;
		double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);
		//fsScale *= (double)textComponent.get_scale();

		//const TextDimensions& dimensions = textComponent.get_dimensions();
		//pRect->Min = { -dimensions.Width / 2.0f, -dimensions.Height / 2.0f };
		//pRect->Max = { dimensions.Width / 2.0f, dimensions.Height / 2.0f };

		//if (pRect)
		//{
		//	TextComponent::HorizontalAlign horizontalAlignment = textComponent.HorizontalAlignment;
		//	TextComponent::VerticalAlign verticalAlignment = textComponent.VerticalAlignment;
		//	// Alignment
		//	switch (horizontalAlignment)
		//	{
		//	case TextComponent::HorizontalAlign::Left:
		//		x = pRect->Min.x;
		//		break;
		//	case TextComponent::HorizontalAlign::Right:
		//		x = pRect->Max.x - dimensions.Width;
		//		break;
		//	case TextComponent::HorizontalAlign::Center:
		//		x = (pRect->Max.x + pRect->Min.x) - dimensions.Width / 2.0f;
		//		break;
		//	}
		//
		//	switch (verticalAlignment)
		//	{
		//	case TextComponent::VerticalAlign::Top:
		//		y = pRect->Max.y - dimensions.Height;
		//		break;
		//	case TextComponent::VerticalAlign::Bottom:
		//		y = pRect->Min.y + metrics.underlineThickness;
		//		break;
		//	case TextComponent::VerticalAlign::Center:
		//		y = (pRect->Max.y + pRect->Min.y) - dimensions.Height / 2.0f;
		//		break;
		//	}
		//}

		const uint32_t TabWidth = 4;
		float spaceAdvance = (float)fontGeometry.getGlyph(' ')->getAdvance();
		float tabColumnWidth = spaceAdvance * TabWidth;

		uint32_t indexCount = 0;
		for (size_t i = 0; i < text.length(); i++)
		{
			char currentChar = text[i];

			// Handle newlines, spaces, tab, etc (don't need to draw an empty quad)
			switch (currentChar)
			{
			case '\r': continue;
			case '\n':
			{
				x = 0.0f;
				y -= fsScale * metrics.lineHeight + lineHeight;
				continue;
			}
			case ' ':
			{
				x += fsScale * spaceAdvance + tracking;
				continue;
			}
			case '\t':
			{
				// TODO: fix tabs
				x += (fsScale * spaceAdvance + tracking) * TabWidth;
				continue;
			}
			}

			const msdf_atlas::GlyphGeometry* glyph = fontGeometry.getGlyph(currentChar);
			if (!glyph)
			{
				LOG("couldn't find glyph for character '{}' in font '{}'", currentChar, fontGeometry.getName());
				continue;
			}

			// quad bounds
			double planeL, planeB, planeR, planeT;
			glyph->getQuadPlaneBounds(planeL, planeB, planeR, planeT);
			planeL *= fsScale;
			planeR *= fsScale;
			planeB *= fsScale;
			planeT *= fsScale;
			planeL += x;
			planeR += x;
			planeB += y;
			planeT += y;

			Float2 quadMin = { (float)planeR, (float)planeB };
			Float2 quadMax = { (float)planeL, (float)planeT };

			// atlas bounds
			double atlasL, atlasB, atlasR, atlasT;
			glyph->getQuadAtlasBounds(atlasL, atlasB, atlasR, atlasT);
			float texelWidth = 1.0f / atlas->get_width();
			float texelHeight = 1.0f / atlas->get_height();
			atlasL *= texelWidth, atlasB *= texelHeight, atlasR *= texelWidth, atlasT *= texelHeight;

			Float2 texCoordMin = { (float)atlasR, (float)atlasB };
			Float2 texCoordMax = { (float)atlasL, (float)atlasT };

			// TL
			s_CurrentVertex->position = { quadMin.x, quadMax.y };
			s_CurrentVertex->texCoord = { texCoordMin.x, texCoordMax.y };
			s_CurrentVertex++;
			// TR
			s_CurrentVertex->position = quadMax;
			s_CurrentVertex->texCoord = texCoordMax;
			s_CurrentVertex++;
			// BR
			s_CurrentVertex->position = { quadMax.x, quadMin.y };
			s_CurrentVertex->texCoord = { texCoordMax.x, texCoordMin.y };
			s_CurrentVertex++;
			// BL
			s_CurrentVertex->position = quadMin;
			s_CurrentVertex->texCoord = texCoordMin;
			s_CurrentVertex++;

			// advance
			if (i + 1 < text.length())
			{
				double advance = glyph->getAdvance();
				char nextChar = text[i + 1];
				fontGeometry.getAdvance(advance, currentChar, nextChar);

				x += fsScale * advance + tracking;
			}

			indexCount += 6;
		}

		atlas->bind();
		s_TextVAO->bind();
		size_t vertexCount = (s_CurrentVertex - s_VertexData);
		s_TextVBO->set_data(s_VertexData, vertexCount * sizeof(TextVertex));

		Graphics::draw_indexed(indexCount);
		s_CurrentVertex = s_VertexData;
	}

}