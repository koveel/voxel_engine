#include "pch.h"

#include "Shader.h"
#include "RenderPipeline.h"
#include "Graphics.h"

namespace Engine {	

	void RenderPipeline::begin(const Matrix4& view, const Matrix4& projection)
	{
		m_ViewMatrix = view;
		m_ProjectionMatrix = projection;
		m_ViewProjectionMatrix = projection * view;
	}

	void RenderPipeline::init_pass(const RenderPass& pass)
	{
		auto shader = pass.pShader;
		if (shader) {
			shader->bind();
			shader->set_matrix("u_ViewProjection", m_ViewProjectionMatrix);
		}

		Graphics::set_face_cull(pass.CullFace);
		Graphics::set_depth_test(pass.Depth.Test);
		Graphics::set_depth_mask(pass.Depth.Write);
		Graphics::set_stencil_test(pass.Stencil.Test);

		if (pass.Stencil.Mask != 0x00)
		{
			const auto& stencilFunc = pass.Stencil.Func;
			Graphics::set_stencil_func(stencilFunc.Test, stencilFunc.Ref, stencilFunc.Mask);

			const auto& ops = pass.Stencil.FaceOp;
			if (ops[0].Use)
			{
				const auto& front = ops[0];
				Graphics::set_face_stencil_op(Face::Front, front.onStencilFail, front.onStencilPassDepthFail, front.onStencilPassDepthPass);
			}
			if (ops[1].Use)
			{
				const auto& back = ops[1];
				Graphics::set_face_stencil_op(Face::Back, back.onStencilFail, back.onStencilPassDepthFail, back.onStencilPassDepthPass);
			}
		}

		//glBlendFunc(pass.Blend.SFactor, pass.Blend.DFactor);
		Graphics::toggle_blend(pass.Blend.Enable);
	}

}