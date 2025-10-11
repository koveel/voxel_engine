#include "pch.h"

#include "windowing/Window.h"
#include "App.h"

#include "rendering/VertexArray.h"
#include "rendering/Shader.h"
#include "rendering/Pipeline.h"

#include "utils/CameraController.h"
#include "voxel/VoxelMesh.h"
#include <glad/glad.h>

#include "gui/Font.h"

#include "voxel/VoxelEntity.h"
#include "rendering/scene/SceneRenderer.h"

namespace Engine {

	App* App::m_Instance = nullptr;

	App::App(const std::string& window_name, uint32_t window_width, uint32_t window_height)
	{
		ASSERT(!m_Instance);

		m_Instance = this;

		m_Window = std::make_unique<Window>(window_name, window_width, window_height);
	}

	App::~App()
	{
	}

	static Camera camera = Camera(ProjectionType::Perspective);
	static CameraController cameraController;

	static std::unique_ptr<Shader> LightShader;
	static std::unique_ptr<Shader> DefaultMeshShader;
	static std::unique_ptr<Shader> CompositeShader;
	static std::unique_ptr<Shader> BlitShader;
	static std::unique_ptr<Shader> Shader_NoFragment;

	static std::unique_ptr<Framebuffer> s_Framebuffer;

	static std::unique_ptr<Shader> s_TextShader;

	static std::unique_ptr<Font> s_Font;

	static Matrix4 get_view_projection()
	{
		Matrix4 cameraView = cameraController.get_view();
		return camera.get_projection() * cameraView;
	}

	void App::run()
	{
		Graphics::init();
		cameraController.m_Camera = &camera;
		//cameraController.m_TargetPosition = { -1.7426194f, 0.00055065745f, 0.12893073f };
		cameraController.m_TargetPosition = { 0.0f, 0.0f, 0.0f };
		cameraController.m_TargetEuler = { 0.46907043f, -86.50404f, 0.0f };

		LightShader = Shader::create("resources/shaders/LightShader.glsl");
		DefaultMeshShader = Shader::create("resources/shaders/DefaultShader.glsl");
		SceneRenderer::s_VoxelMeshShader = Shader::create("resources/shaders/VoxelShader.glsl");
		Shader_NoFragment = Shader::create("resources/shaders/VertexOnlyShader.glsl");
		CompositeShader = Shader::create("resources/shaders/CompositeShader.glsl");
		BlitShader = Shader::create("resources/shaders/BlitShader.glsl");

		auto postprocessShader = Shader::create("resources/shaders/PPShader.glsl");

		//meshes[1] = VoxelMesh::load_from_file("resources/models/gas_tank_22.png");
		//meshes[2] = VoxelMesh::load_from_file("resources/models/truck_35.png");

		s_TextShader = Shader::create("resources/shaders/TextShader.glsl");

		s_Font = Font::load_from_file("resources/fonts/Raleway-Regular.ttf");

		auto startTime = std::chrono::high_resolution_clock::now();
		float lastElapsed = 0.0f;

		{
			uint32_t screenWidth = m_Window->get_width();
			uint32_t screenHeight = m_Window->get_height();

			FramebufferDescriptor descriptor;
			descriptor.ColorAttachments = {
				{ screenWidth, screenHeight, TextureFormat::RGBA8 }, // albedo
				{ screenWidth, screenHeight, TextureFormat::RGB8S }, // normal

				{ screenWidth, screenHeight, TextureFormat::RGB8S, false }, // normal last frame
				{ screenWidth, screenHeight, TextureFormat::R8, false }, // AO accumulation tex 1
				{ screenWidth, screenHeight, TextureFormat::R8, false }, // AO accumulation tex 2
				{ screenWidth, screenHeight, TextureFormat::R16, false }, // depth last frame

				{ screenWidth, screenHeight, TextureFormat::RGBA8 }, // lighting
			};
			FramebufferDescriptor::DepthStencilAttachment depthStencilAttachment = {
				screenWidth, screenHeight, 24, true
			};
			descriptor.pDepthStencilAttachment = &depthStencilAttachment;

			s_Framebuffer = Framebuffer::create(descriptor);
			s_Framebuffer->bind();

			float v = 1.0f;
			glClearTexImage(s_Framebuffer->m_ColorAttachments[3]->get_handle(), 0, GL_RED, GL_FLOAT, &v);
			glClearTexImage(s_Framebuffer->m_ColorAttachments[4]->get_handle(), 0, GL_RED, GL_FLOAT, &v);
		}


		// CREATE PASSES
		RenderPass geometryPass;
		geometryPass.pShader = SceneRenderer::s_VoxelMeshShader.get();
		geometryPass.Depth.Write = true;

		RenderPass stencilPass;
		stencilPass.pShader = Shader_NoFragment.get();
		stencilPass.CullFace = Face::None;
		stencilPass.Stencil.Mask = 0xff;
		stencilPass.Stencil.Func = {
			StencilTest::Always, 0x00, 0x00
		};
		stencilPass.Stencil.FaceOp[0] = { true, StencilOp::Keep, StencilOp::IncrementWrap, StencilOp::Keep };
		stencilPass.Stencil.FaceOp[1] = { true, StencilOp::Keep, StencilOp::DecrementWrap, StencilOp::Keep };

		RenderPass lightPass;
		lightPass.pShader = LightShader.get();
		lightPass.Depth.Test = DepthTest::Off;
		lightPass.CullFace = Face::Front;
		lightPass.Stencil.Func = { StencilTest::NotEqual, 0x00, 0xff };
		lightPass.Blend = true;

		// Final
		RenderPass aoPass;
		aoPass.pShader = postprocessShader.get();
		aoPass.Depth.Test = DepthTest::Off;

		RenderPass compositePass;
		compositePass.pShader = CompositeShader.get();
		compositePass.Depth.Test = DepthTest::Off;

		RenderPass blitPass;
		blitPass.pShader = BlitShader.get();
		blitPass.Depth.Test = DepthTest::Off;

		auto reloadShaders = [&]()
		{
			postprocessShader.reset();
			postprocessShader = Shader::create("resources/shaders/PPShader.glsl");
			aoPass.pShader = postprocessShader.get();

			LightShader.reset();
			LightShader = Shader::create("resources/shaders/LightShader.glsl");
			lightPass.pShader = LightShader.get();
		};

		// UI
		RenderPass screenspaceUIPass;
		screenspaceUIPass.Depth.Test = DepthTest::Off;
		screenspaceUIPass.Blend = true;
		screenspaceUIPass.CullFace = Face::Back;

		RenderPass debug_geo_pass;
		debug_geo_pass.Depth.Test = DepthTest::Off;
		debug_geo_pass.pShader = DefaultMeshShader.get();

		Pipeline pipeline;

		VoxelEntity gas_tank;
		gas_tank.Mesh = VoxelMesh::load_from_file("resources/models/gas_tank_22.png");
		gas_tank.Transform.Position = { 0.0f, 0.0f, 0.0f };
		gas_tank.Transform.Scale = (Float3)gas_tank.Mesh.m_Texture->get_dimensions() * 0.1f;

		VoxelEntity blue_car;
		blue_car.Mesh = VoxelMesh::load_from_file("resources/models/blue_car_13.png");
		blue_car.Transform.Position = { -4.0f, 0.0f, -2.0f };
		blue_car.Transform.Scale = (Float3)blue_car.Mesh.m_Texture->get_dimensions() * 0.1f;

		VoxelEntity box;
		box.Mesh = VoxelMesh::load_from_file("resources/models/box_120.png");
		box.Transform.Position = { 0.0f, 0.0f, 0.0f };
		box.Transform.Scale = (Float3)box.Mesh.m_Texture->get_dimensions() * 0.1f;

		VoxelEntity transformer;
		transformer.Mesh = VoxelMesh::load_from_file("resources/models/transformer_57.png");
		transformer.Transform.Position = { -5.0f, 1.05f, -4.05f };
		transformer.Transform.Scale = (Float3)transformer.Mesh.m_Texture->get_dimensions() * 0.1f;

		VoxelEntity miniPole;
		miniPole.Mesh = VoxelMesh::load_from_file("resources/models/pole_12.png");
		miniPole.Transform.Position = { -2.5f, 0.0f, -1.8f };
		miniPole.Transform.Scale = (Float3)miniPole.Mesh.m_Texture->get_dimensions() * 0.1f;

		auto blueNoise = Texture2D::load("resources/textures/blue_noise_512.png");
		blueNoise->set_wrap_mode(TextureWrapMode::Repeat);
		blueNoise->set_filter_mode(TextureFilterMode::Point);

		static Matrix4 s_PreviousViewProjection = Matrix4(1.0f);

		uint32_t frameNumber = 0;
		while (m_Window->is_open())
		{
			Input::clear_state();
			Input::update_mouse_delta();
			m_Window->handle_events();

			if (!m_Window->is_minimized())
			{
				cameraController.update(m_DeltaTime);

				// Clear
				//s_Framebuffer->bind();
				//Graphics::clear(, 1.0f);
				s_Framebuffer->clear({ 0.0f });

				Matrix4 view = cameraController.get_view();
				Matrix4 projection = camera.get_projection();
				pipeline.begin(view, projection);
				SceneRenderer::begin_frame(camera, cameraController.get_transform());

				std::vector<VoxelEntity*> ENTITIES_TO_RENDER = {
					&gas_tank, &transformer, &miniPole,
					&box,
				};				

				if (Input::was_key_pressed(Key::P))
				{
					auto& transform = cameraController.m_Transformation;
					LOG("{}, {}, {}", transform.Position.x, transform.Position.y, transform.Position.z);
					LOG("{}, {}, {}", transform.Rotation.x, transform.Rotation.y, transform.Rotation.z);
				}

				static bool b = true;
				if (b) {
					SceneRenderer::generate_shadow_map(ENTITIES_TO_RENDER);
					b = false;
				}
				b = Input::was_key_pressed(Key::RightMouse);

				if (Input::was_key_pressed(Key::R) && Input::is_key_down(Key::Control))
				{
					reloadShaders();
				}

				// GEOMETRY PASS
				pipeline.submit_pass(geometryPass, [&]()
				{
					static bool show_shadow_map = false;
					if (Input::was_key_pressed(Key::M)) show_shadow_map = !show_shadow_map;
					if (show_shadow_map)
						SceneRenderer::draw_shadow_map();

					for (auto e : ENTITIES_TO_RENDER)
						SceneRenderer::draw_voxel_entity(*e);
				});

				static Float3 lightPos = { -2.0f, 0.0f, -2.0f };
				// LIGHTING SHT
				{
					static float radius = 4.0f;

					{
						if (Input::is_key_down(Key::UpArrow))
							lightPos.z -= m_DeltaTime / 2.0f;
						if (Input::is_key_down(Key::DownArrow))
							lightPos.z += m_DeltaTime / 2.0f;
						if (Input::is_key_down(Key::LeftArrow))
							lightPos.x -= m_DeltaTime / 2.0f;
						if (Input::is_key_down(Key::RightArrow))
							lightPos.x += m_DeltaTime / 2.0f;
					}

					auto lightTransformation = Transformation(lightPos, Float3(0.0f), Float3(radius * 2, radius * 2, radius * 2)).get_transform();
					//static float attConst = 0.7f, attLinear = -0.1f, attExp = -0.13f;
					static float intensity = 0.9f;

					// STENCIL PASS
					pipeline.submit_pass(stencilPass, [&]()
					{
						glDrawBuffer(GL_NONE);
						glClear(GL_STENCIL_BUFFER_BIT);
					
						Shader_NoFragment->set_matrix("u_Transformation", lightTransformation);
					
						Graphics::draw_sphere(); // lighting volume mesh
					});

					s_Framebuffer->bind(); // stencil pass fucks up the draw buffers

					// LIGHT PASS
					pipeline.submit_pass(lightPass, [&]()
					{
						glBlendFunc(GL_ONE, GL_ONE);

						// Matrices
						LightShader->set_matrix("u_Transformation", lightTransformation);
						LightShader->set_matrix("u_InverseView", glm::inverse(view));
						LightShader->set_matrix("u_InverseProjection", glm::inverse(projection));

						// Light params
						//LightShader->set_float("AttConst", attConst);
						//LightShader->set_float("AttLinear", attLinear);
						//LightShader->set_float("AttExp", attExp);
						LightShader->set_float("u_LightIntensity", intensity);
						LightShader->set_float("u_LightRadius", radius);

						Float3 color = { 0.9f, 0.8f, 0.05f };
						LightShader->set_float3("u_Color", color);
						LightShader->set_float3("u_VolumeCenter", lightPos);
						LightShader->set_float2("u_ViewportDims", { m_Window->get_width(), m_Window->get_height() });
						LightShader->set_int("u_FrameNumber", frameNumber);
						s_Framebuffer->m_DepthStencilAttachment->bind(0);
						s_Framebuffer->m_ColorAttachments[1]->bind(2);

						Graphics::draw_sphere(); // lighting volume mesh
					});
				}

				Texture2D* ao_read_texture = nullptr, *ao_write_texture = nullptr;
				// AO PASS
				pipeline.submit_pass(aoPass, [&]()
				{
					// HANDLE AO PING PONG MUDAFUKA
					{
						uint32_t readIndex = frameNumber % 2;
						uint32_t writeIndex = 1 - readIndex;

						const uint32_t aoIndex = 3;
						uint32_t fbo = s_Framebuffer->get_handle();
						const auto& read = s_Framebuffer->m_ColorAttachments[aoIndex + readIndex];
						const auto& write = s_Framebuffer->m_ColorAttachments[aoIndex + writeIndex];
						glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT3, write->get_handle(), 0);
						ASSERT(glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

						ao_read_texture = read.get();
						ao_write_texture = write.get();
						read->bind(6);
					}

						// 1 u_PreviousDepth;
						// 2 u_Albedo;
						// 3 u_Normal;
						// 4 u_PreviousNormal;
						// 5 u_AmbientAccumulation;
						// 6 u_Lighting;
						// 7 u_ShadowMap;
						// 8 u_BlueNoiseTexture;

					s_Framebuffer->m_DepthStencilAttachment->bind(0);
					SceneRenderer::get_shadow_map()->bind(1);
					s_Framebuffer->m_ColorAttachments[5]->bind(2);
				
					s_Framebuffer->m_ColorAttachments[0]->bind(3);
					s_Framebuffer->m_ColorAttachments[1]->bind(4);
					s_Framebuffer->m_ColorAttachments[2]->bind(5);
					blueNoise->bind(9);

					postprocessShader->set_float("u_Time", m_ElapsedTime);
					postprocessShader->set_matrix("u_InverseView", glm::inverse(view));
					postprocessShader->set_matrix("u_InverseProjection", glm::inverse(projection));
					postprocessShader->set_matrix("u_PrevFrameViewProjection", s_PreviousViewProjection);
					postprocessShader->set_float3("u_CameraPos", cameraController.m_Transformation.Position);
					postprocessShader->set_int("u_FrameNumber", frameNumber);

					Graphics::draw_fullscreen_triangle(postprocessShader);
				});

				// BLIT A BIT O DAT SHIT
				pipeline.submit_pass(blitPass, [&]()
				{
					s_Framebuffer->m_DepthStencilAttachment->bind(0);
					s_Framebuffer->m_ColorAttachments[1]->bind(1);
					Graphics::draw_fullscreen_triangle(BlitShader);
				});

				// READS AO ACCUM
				pipeline.submit_pass(compositePass, [&]()
				{
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					s_Framebuffer->m_ColorAttachments[0]->bind(0);
					s_Framebuffer->m_ColorAttachments[1]->bind(1);
					ao_write_texture->bind(2);
					s_Framebuffer->m_ColorAttachments[6]->bind(3);

					static int shaderOutputColor = 0;
					if (Input::was_key_pressed(Key::A_1)) shaderOutputColor = 0; // final
					if (Input::was_key_pressed(Key::A_2)) shaderOutputColor = 1; // albedo
					if (Input::was_key_pressed(Key::A_3)) shaderOutputColor = 2; // normal
					if (Input::was_key_pressed(Key::A_4)) shaderOutputColor = 3; // ao
					CompositeShader->set_int("u_Output", shaderOutputColor);

					Graphics::draw_fullscreen_triangle(CompositeShader);
				});

				pipeline.submit_pass(debug_geo_pass, [&]()
				{
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					Matrix4 debuglighttransform = Transformation(lightPos, {}, { 0.2f, 0.2f, 0.2f }).get_transform();
					DefaultMeshShader->set_matrix("u_Transformation", debuglighttransform);
				
					Graphics::draw_sphere();
				});

				Float2 viewport = { (float)m_Window->get_width(), (float)m_Window->get_height() };
				Matrix4 pixelProjection = glm::ortho(0.0f, viewport.x, 0.0f, viewport.y, -1.0f, 1.0f);
				pipeline.submit_pass(screenspaceUIPass, [&]()
				{
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					s_TextShader->bind();
					s_TextShader->set_matrix("u_ViewProjection", pixelProjection);
					{
						s_TextShader->set_matrix("u_Transformation", 
							Transformation({ 25.0f, viewport.y - 80.0f, 0.0f }, {}, { 40.0f, 40.0f, 1.0f }).get_transform());
						Graphics::draw_text(std::format("ms: {}", m_DeltaTime * 1000.0f), s_Font);
					}
					{
						s_TextShader->set_matrix("u_Transformation",
							Transformation({ 25.0f, viewport.y - 165.0f, 0.0f }, {}, { 40.0f, 40.0f, 1.0f }).get_transform());
						Graphics::draw_text(std::format("fps: {}", 1.0f / m_DeltaTime), s_Font);
					}
				});

				s_PreviousViewProjection = pipeline.m_ViewProjectionMatrix;
				frameNumber++;
			}

			// Time
			auto current = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> elapsedDuration = current - startTime;
			m_DeltaTime = elapsedDuration.count() - lastElapsed;
			m_ElapsedTime += m_DeltaTime;
			lastElapsed = elapsedDuration.count();

			m_Window->swap_buffers();
		}
	}

	void App::close()
	{
	}

	bool App::is_running() const
	{
		return m_Window->is_open();
	}

	Window* App::get_window() const
	{
		return m_Window.get();
	}

	void App::handle_event(Event& event)
	{
		EventMessenger messenger(event);
		messenger.send<WindowResizeEvent>(DELEGATE(App::on_window_resize));

		messenger.send<KeyPressEvent>(DELEGATE(App::on_key_press));
		messenger.send<KeyReleaseEvent>(DELEGATE(App::on_key_release));

		messenger.send<MouseButtonPressEvent>(DELEGATE(App::on_mouse_button_press));
		messenger.send<MouseButtonReleaseEvent>(DELEGATE(App::on_mouse_button_release));
		messenger.send<MouseMoveEvent>(DELEGATE(App::on_mouse_move));
		messenger.send<MouseScrollEvent>(DELEGATE(App::on_mouse_scroll));

		cameraController.on_event(event);
	}

	void App::on_window_resize(WindowResizeEvent& e)
	{
		Graphics::resize_viewport(e.width, e.height);

		s_Framebuffer->resize(e.width, e.height);

		camera.set_aspect_ratio((float)e.width / (float)e.height);
	}

	void App::on_key_press(KeyPressEvent& e)
	{
		Input::s_KeysDown[(uint32_t)e.key] = 1;
		if (!e.repeat)
			Input::s_KeysPressed[(uint32_t)e.key] = 1;
	}
	void App::on_key_release(KeyReleaseEvent& e)
	{
		Input::s_KeysDown[(uint32_t)e.key] = 0;
		Input::s_KeysReleased[(uint32_t)e.key] = 1;
	}
	void App::on_mouse_button_press(MouseButtonPressEvent& e)
	{
		bool first = Input::s_KeysDown[(uint32_t)e.button] == 0;
		Input::s_KeysDown[(uint32_t)e.button] = 1;
		if (first)
			Input::s_KeysPressed[(uint32_t)e.button] = 1;
	}
	void App::on_mouse_button_release(MouseButtonReleaseEvent& e)
	{
		Input::s_KeysDown[(uint32_t)e.button] = 0;
		Input::s_KeysReleased[(uint32_t)e.button] = 1;
	}
	void App::on_mouse_move(MouseMoveEvent& e)
	{
		Input::s_MouseX = (float)e.x;
		Input::s_MouseY = (float)e.y;
	}
	void App::on_mouse_scroll(MouseScrollEvent& e)
	{
		camera.set_ortho_size(camera.get_ortho_size() - (float)e.delta * 0.25f);
	}

}