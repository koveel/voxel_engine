#pragma once

#include "Graphics.h"
#include "Framebuffer.h"

namespace Engine {

	// threw this together in a jif, improve later
	struct RenderPass
	{
		Matrix4 ViewMatrix;
		Matrix4 ProjectionMatrix;

		Shader* pShader = nullptr;

		// Culling, depth & stencil testing
		Face CullFace = Face::Back;
		struct
		{
			bool Write = false;
			//DepthTest Test = DepthTest::Less;
			DepthTest Test = DepthTest::Greater;
		} Depth;
		struct
		{
			bool Test = false;
			uint8_t Mask = 0x00;

			struct
			{
				StencilTest Test;
				uint8_t Ref = 0x00;
				uint8_t Mask = 0xff;
			} Func;

			struct
			{
				bool Use = false;
				StencilOp onStencilFail;
				StencilOp onStencilPassDepthFail;
				StencilOp onStencilPassDepthPass;
			} FaceOp[2]; // 0 = Face::Front, 1 = Face::Back
		} Stencil;

		struct
		{
			bool Enable = false;
		} Blend;
	};

	class RenderPipeline
	{
	public:
		void begin(const Matrix4& view, const Matrix4& projection);

		template<typename F>
		void submit_pass(const RenderPass& pass, F command)
		{
			init_pass(pass);
			command();
		}
	private:
		void init_pass(const RenderPass& pass);
	public:
		Matrix4 m_ViewMatrix = Matrix4(1.0f), m_ProjectionMatrix = Matrix4(1.0f), m_ViewProjectionMatrix = Matrix4(1.0f);
	};

}