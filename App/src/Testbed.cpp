#include "pch.h"
#include "App.h"
#include "utils/CameraController.h"

#include "rendering/Shader.h"
#include "rendering/scene/SceneRenderer.h"
#include "rendering/Pipeline.h"

#include "windowing/Window.h"

#include "gui/Font.h"

using namespace Engine;

static Camera camera = Camera(ProjectionType::Perspective);
static CameraController cameraController;

static std::unique_ptr<Framebuffer> s_Framebuffer;

static std::unique_ptr<Shader> LightShader;
static std::unique_ptr<Shader> DefaultMeshShader;
static std::unique_ptr<Shader> CompositeShader;
static std::unique_ptr<Shader> BlitShader;
static std::unique_ptr<Shader> Shader_NoFragment;
static std::unique_ptr<Shader> s_SpriteShader;
static std::unique_ptr<Shader> s_TextShader;
static std::unique_ptr<Shader> postprocessShader;

static Pipeline pipeline;
static RenderPass geometryPass;
static RenderPass stencilPass;
static RenderPass lightPass;
static RenderPass aoPass;
static RenderPass compositePass;
static RenderPass blitPass;
static RenderPass debug_geo_pass;
static RenderPass screenspaceUIPass;

static std::unique_ptr<Font> s_Font;

static VoxelEntity gas_tank;
static VoxelEntity blue_car;
static VoxelEntity box;
static VoxelEntity transformer;
static VoxelEntity miniPole;

static std::unique_ptr<Texture2D> blueNoise;
static std::unique_ptr<Texture2D> crosshair;

static Matrix4 s_PreviousViewProjection = Matrix4(1.0f);

static const std::vector<VoxelEntity*> ENTITIES_TO_RENDER = {
	&gas_tank, &transformer,// &miniPole,
	&box,
};

static void reload_all_shaders()
{
	LightShader = Shader::create("resources/shaders/LightShader.glsl");
	DefaultMeshShader = Shader::create("resources/shaders/DefaultShader.glsl");
	SceneRenderer::s_VoxelMeshShader = Shader::create("resources/shaders/VoxelShader.glsl");
	Shader_NoFragment = Shader::create("resources/shaders/VertexOnlyShader.glsl");
	CompositeShader = Shader::create("resources/shaders/CompositeShader.glsl");
	BlitShader = Shader::create("resources/shaders/BlitShader.glsl");
	s_SpriteShader = Shader::create("resources/shaders/SpriteShader.glsl");
	postprocessShader = Shader::create("resources/shaders/PPShader.glsl");
	s_TextShader = Shader::create("resources/shaders/TextShader.glsl");
}

static void init_renderpass()
{
	geometryPass.pShader = SceneRenderer::s_VoxelMeshShader.get();
	geometryPass.Depth.Write = true;

	stencilPass.pShader = Shader_NoFragment.get();
	stencilPass.CullFace = Face::None;
	stencilPass.Stencil.Mask = 0xff;
	stencilPass.Stencil.Func = {
		StencilTest::Always, 0x00, 0x00
	};
	stencilPass.Stencil.FaceOp[0] = { true, StencilOp::Keep, StencilOp::IncrementWrap, StencilOp::Keep };
	stencilPass.Stencil.FaceOp[1] = { true, StencilOp::Keep, StencilOp::DecrementWrap, StencilOp::Keep };

	lightPass.pShader = LightShader.get();
	lightPass.Depth.Test = DepthTest::Off;
	lightPass.CullFace = Face::Front;
	lightPass.Stencil.Func = { StencilTest::NotEqual, 0x00, 0xff };
	lightPass.Blend.Enable = true;

	// Final
	aoPass.pShader = postprocessShader.get();
	aoPass.Depth.Test = DepthTest::Off;

	compositePass.pShader = CompositeShader.get();
	compositePass.Depth.Test = DepthTest::Off;

	blitPass.pShader = BlitShader.get();
	blitPass.Depth.Test = DepthTest::Off;

	debug_geo_pass.Depth.Test = DepthTest::Off;
	debug_geo_pass.pShader = DefaultMeshShader.get();
	debug_geo_pass.CullFace = Face::Front;
	debug_geo_pass.Blend.Enable = true;

	screenspaceUIPass.Depth.Test = DepthTest::Off;
	screenspaceUIPass.Blend.Enable = true;
	screenspaceUIPass.CullFace = Face::Back;
}

static void load_models()
{
	gas_tank.Mesh = VoxelMesh::load_from_file("resources/models/gas_tank_22.png");
	gas_tank.Transform.Position = { 0.0f, 0.0f, 0.0f };
	gas_tank.Transform.Scale = (Float3)gas_tank.Mesh.m_Texture->get_dimensions() * 0.1f;

	blue_car.Mesh = VoxelMesh::load_from_file("resources/models/blue_car_13.png");
	blue_car.Transform.Position = { -4.0f, 0.0f, -2.0f };
	blue_car.Transform.Scale = (Float3)blue_car.Mesh.m_Texture->get_dimensions() * 0.1f;

	box.Mesh = VoxelMesh::load_from_file("resources/models/box_120.png");
	box.Transform.Position = { 0.0f, 0.0f, 0.0f };
	box.Transform.Scale = (Float3)box.Mesh.m_Texture->get_dimensions() * 0.1f;

	transformer.Mesh = VoxelMesh::load_from_file("resources/models/transformer_57.png");
	transformer.Transform.Position = { -5.0f, 1.05f, -4.05f };
	transformer.Transform.Scale = (Float3)transformer.Mesh.m_Texture->get_dimensions() * 0.1f;

	miniPole.Mesh = VoxelMesh::load_from_file("resources/models/pole_12.png");
	miniPole.Transform.Position = { -2.5f, 0.0f, -1.8f };
	miniPole.Transform.Scale = (Float3)miniPole.Mesh.m_Texture->get_dimensions() * 0.1f;
}

void testbed_start(App& app)
{
	Window* window = app.get_window();

	cameraController.m_Camera = &camera;
	cameraController.m_TargetPosition = { -3.0f, 0.0f, 0.0f };
	cameraController.m_TargetEuler = { 0.0f, -90.0f, 0.0f };

	reload_all_shaders();

	// FRAMEBUFFER
	{
		uint32_t screenWidth = window->get_width();
		uint32_t screenHeight = window->get_height();

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
		s_Framebuffer->m_ColorAttachments[3]->clear_to(&v);
		s_Framebuffer->m_ColorAttachments[4]->clear_to(&v);
	}

	init_renderpass();

	load_models();

	blueNoise = Texture2D::load("resources/textures/blue_noise_512.png");
	blueNoise->set_wrap_mode(TextureWrapMode::Repeat);
	blueNoise->set_filter_mode(TextureFilterMode::Point);
	
	crosshair = Texture2D::load("resources/textures/crosshair0.png");
	crosshair->set_wrap_mode(TextureWrapMode::Clamp);
	crosshair->set_filter_mode(TextureFilterMode::Linear);

	s_Font = Font::load_from_file("resources/fonts/Raleway-Regular.ttf");
}

void testbed_update(App& app)
{
	Window* window = app.get_window();
	float deltaTime = app.get_delta_time();
	float elapsedTime = app.get_elapsed_time();
	uint32_t frameNumber = (uint32_t)app.get_frame();

	cameraController.update(deltaTime);

	// Clear
	s_Framebuffer->clear({ 0.0f });

	Matrix4 view = cameraController.get_view();
	Matrix4 projection = camera.get_projection();
	pipeline.begin(view, projection);
	SceneRenderer::begin_frame(camera, cameraController.get_transform());	

	if (Input::was_key_pressed(Key::R) || frameNumber == 0) {
		SceneRenderer::generate_shadow_map(ENTITIES_TO_RENDER);
	}

	if (Input::was_key_pressed(Key::R) && Input::is_key_down(Key::Control))
		reload_all_shaders();

	// VIEW RAY TRACE
	TracedRay viewRay;
	{
		Transformation& transform = cameraController.get_transform();
		Float3 origin = transform.Position;

		float pitch = glm::radians(transform.Rotation.x), yaw = glm::radians(transform.Rotation.y);
		Float3 forward = Float3(
			glm::cos(pitch) * -glm::sin(yaw),
			glm::sin(pitch),
			glm::cos(pitch) * -glm::cos(yaw)
		);

		viewRay = SceneRenderer::trace_ray_through_scene(origin, forward, 10.0f);
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
	if (false)
	{
		static float radius = 4.0f;

		{
			if (Input::is_key_down(Key::UpArrow))
				lightPos.z -= deltaTime / 2.0f;
			if (Input::is_key_down(Key::DownArrow))
				lightPos.z += deltaTime / 2.0f;
			if (Input::is_key_down(Key::LeftArrow))
				lightPos.x -= deltaTime / 2.0f;
			if (Input::is_key_down(Key::RightArrow))
				lightPos.x += deltaTime / 2.0f;
		}

		auto lightTransformation = Transformation(lightPos, Float3(0.0f), Float3(radius * 2, radius * 2, radius * 2)).get_transform();
		//static float attConst = 0.7f, attLinear = -0.1f, attExp = -0.13f;
		static float intensity = 1.0f;

		// STENCIL PASS
		pipeline.submit_pass(stencilPass, [&]()
		{
			Graphics::reset_draw_buffers();

			Shader_NoFragment->set_matrix("u_Transformation", lightTransformation);

			Graphics::draw_sphere(); // lighting volume mesh
		});

		s_Framebuffer->bind(); // stencil pass fucks up the draw buffers

		// LIGHT PASS
		pipeline.submit_pass(lightPass, [&]()
		{
			//glBlendFunc(GL_ONE, GL_ONE);

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

			Float3 color = { 0.9f, 0.9f, 0.05f };
			LightShader->set_float3("u_Color", color);
			LightShader->set_float3("u_VolumeCenter", lightPos);
			LightShader->set_float2("u_ViewportDims", { window->get_width(), window->get_height() });
			LightShader->set_int("u_FrameNumber", frameNumber);
			s_Framebuffer->m_DepthStencilAttachment->bind(0);
			s_Framebuffer->m_ColorAttachments[1]->bind(2);

			Graphics::draw_sphere(); // lighting volume mesh
		});
	}

	Texture2D* ao_read_texture = nullptr, * ao_write_texture = nullptr;
	// AO PASS
	pipeline.submit_pass(aoPass, [&]()
	{
		// HANDLE AO PING PONG
		{
			uint32_t readIndex = frameNumber % 2;
			uint32_t writeIndex = 1 - readIndex;

			const uint32_t aoIndex = 3;
			uint32_t fbo = s_Framebuffer->get_handle();
			const auto& read = s_Framebuffer->m_ColorAttachments[aoIndex + readIndex];
			const auto& write = s_Framebuffer->m_ColorAttachments[aoIndex + writeIndex];
			s_Framebuffer->set_index_handle(3, write->get_handle());

			ao_read_texture = read.get();
			ao_write_texture = write.get();
			read->bind(6);
		}

		s_Framebuffer->m_DepthStencilAttachment->bind(0);
		SceneRenderer::get_shadow_map()->bind(1);
		s_Framebuffer->m_ColorAttachments[5]->bind(2);

		s_Framebuffer->m_ColorAttachments[0]->bind(3);
		s_Framebuffer->m_ColorAttachments[1]->bind(4);
		s_Framebuffer->m_ColorAttachments[2]->bind(5);
		blueNoise->bind(9);

		postprocessShader->set_float("u_Time", elapsedTime);
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
		Framebuffer::unbind();
		s_Framebuffer->m_ColorAttachments[0]->bind(0);
		s_Framebuffer->m_ColorAttachments[1]->bind(1);
		ao_write_texture->bind(2);
		s_Framebuffer->m_ColorAttachments[6]->bind(3);

		static int shaderOutputColor = 0;
		if (Input::was_key_pressed(Key::A_1)) shaderOutputColor = 0;
		if (Input::was_key_pressed(Key::A_2)) shaderOutputColor = 1;
		if (Input::was_key_pressed(Key::A_3)) shaderOutputColor = 2;
		if (Input::was_key_pressed(Key::A_4)) shaderOutputColor = 3;
		if (Input::was_key_pressed(Key::A_5)) shaderOutputColor = 4;
		if (Input::was_key_pressed(Key::A_6)) shaderOutputColor = 5;
		if (Input::was_key_pressed(Key::A_7)) shaderOutputColor = 6;
		CompositeShader->set_int("u_Output", shaderOutputColor);

		Graphics::draw_fullscreen_triangle(CompositeShader);
	});

	Float2 viewport = { (float)window->get_width(), (float)window->get_height() };
	pipeline.submit_pass(debug_geo_pass, [&]()
	{
		Framebuffer::unbind();

		Float3 center = Float3(viewRay.cell) * 0.1f + 0.05f;
		Float3 scale = Float3(0.1f) + 0.001f;

		Matrix4 debuglighttransform = Transformation(center, {}, scale).get_transform();
		DefaultMeshShader->set_float4("u_Color", { 1.0f, 1.0f, 1.0f, 0.4f });
		DefaultMeshShader->set_matrix("u_Transformation", debuglighttransform);
		DefaultMeshShader->set_float2("u_ViewportDims", viewport);
		s_Framebuffer->m_DepthStencilAttachment->bind(0);

		Graphics::draw_cube();
	});

	Matrix4 pixelProjection = glm::ortho(0.0f, viewport.x, 0.0f, viewport.y, -1.0f, 1.0f);
	pipeline.submit_pass(screenspaceUIPass, [&]()
	{
		s_TextShader->bind();
		s_TextShader->set_matrix("u_ViewProjection", pixelProjection);
		{
			s_TextShader->set_matrix("u_Transformation",
				Transformation({ 25.0f, viewport.y - 80.0f, 0.0f }, {}, { 40.0f, 40.0f, 1.0f }).get_transform());
			Graphics::draw_text(std::format("ms: {}", deltaTime * 1000.0f), s_Font);
		}
		{
			s_TextShader->set_matrix("u_Transformation",
				Transformation({ 25.0f, viewport.y - 165.0f, 0.0f }, {}, { 40.0f, 40.0f, 1.0f }).get_transform());
			Graphics::draw_text(std::format("fps: {}", 1.0f / deltaTime), s_Font);
		}

		s_SpriteShader->bind();
		s_SpriteShader->set_float4("u_Tint", { 0.0f, 0.0f, 0.0f, 1.0f });
		crosshair->bind(0);
		s_SpriteShader->set_matrix("u_ViewProjection", pixelProjection);

		float aspect = viewport.y / viewport.x;
		Matrix4 transform = Transformation({ viewport.x / 2.0f, viewport.y / 2.0f, 0.0f }, {},
			{ 100.0f, 100.0f, 1.0f }).get_transform();
		s_SpriteShader->set_matrix("u_Transformation", transform);
		Graphics::draw_quad();
	});

	s_PreviousViewProjection = pipeline.m_ViewProjectionMatrix;
	frameNumber++;
}

void testbed_stop(App& app)
{

}

void testbed_window_resized(WindowResizeEvent& e)
{
	s_Framebuffer->resize(e.width, e.height);
	camera.set_aspect_ratio((float)e.width / (float)e.height);
}

void testbed_mouse_scrolled(MouseScrollEvent& e)
{
}

void testbed_event(Event& event)
{
	EventMessenger messenger(event);
	messenger.send<WindowResizeEvent>(testbed_window_resized);

	cameraController.on_event(event);
}