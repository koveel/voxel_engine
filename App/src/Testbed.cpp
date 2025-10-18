#include "pch.h"
#include "App.h"
#include "utils/CameraController.h"

#include "rendering/Shader.h"
#include "rendering/scene/SceneRenderer.h"
#include "rendering/RenderPipeline.h"

#include "windowing/Window.h"

#include "gui/Font.h"

using namespace Engine;

static Camera camera = Camera(ProjectionType::Perspective);
static CameraController cameraController;

//static owning_ptr<Framebuffer> s_Framebuffer;

static RenderPipeline pipeline;
static RenderPass rp_Geometry;
static RenderPass rp_Stencil;
static RenderPass rp_Lighting;
static RenderPass rp_AmbientOcclusion;
static RenderPass rp_Composite;
static RenderPass rp_Blit;
static RenderPass rp_DebugGeometry;
static RenderPass rp_ScreenspaceUI;

static owning_ptr<Shader> LightShader;
static owning_ptr<Shader> DefaultMeshShader;
static owning_ptr<Shader> CompositeShader;
static owning_ptr<Shader> BlitShader;
static owning_ptr<Shader> Shader_NoFragment;
static owning_ptr<Shader> s_SpriteShader;
static owning_ptr<Shader> s_TextShader;
static owning_ptr<Shader> postprocessShader;

static owning_ptr<ComputeShader> s_Compute;

static owning_ptr<Font> s_Font;

static VoxelEntity gas_tank;
static VoxelEntity blue_car;
static VoxelEntity box;
static VoxelEntity transformer;
static VoxelEntity miniPole;
static VoxelEntity chunk;

static owning_ptr<Texture2D> blueNoise;
static owning_ptr<Texture2D> crosshair;

static owning_ptr<Texture2D> s_ComputeImageTarget;

static Matrix4 s_PreviousViewProjection = Matrix4(1.0f);

static const std::vector<VoxelEntity*> ENTITIES_TO_RENDER = {
	//&gas_tank, &transformer, &box
	&chunk,
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

	s_Compute = ComputeShader::create("resources/shaders/compute/ComputeAO.glsl");
}

static void init_renderpass()
{
	rp_Geometry.pShader = SceneRenderer::s_VoxelMeshShader.get();
	rp_Geometry.Depth.Write = true;

	rp_Stencil.pShader = Shader_NoFragment.get();
	rp_Stencil.CullFace = Face::None;
	rp_Stencil.Stencil.Mask = 0xff;
	rp_Stencil.Stencil.Func = {
		StencilTest::Always, 0x00, 0x00
	};
	rp_Stencil.Stencil.FaceOp[0] = { true, StencilOp::Keep, StencilOp::IncrementWrap, StencilOp::Keep };
	rp_Stencil.Stencil.FaceOp[1] = { true, StencilOp::Keep, StencilOp::DecrementWrap, StencilOp::Keep };

	rp_Lighting.pShader = LightShader.get();
	rp_Lighting.Depth.Test = DepthTest::Off;
	rp_Lighting.CullFace = Face::Front;
	rp_Lighting.Stencil.Func = { StencilTest::NotEqual, 0x00, 0xff };
	rp_Lighting.Blend.Enable = true;

	// Final
	rp_AmbientOcclusion.pShader = postprocessShader.get();
	rp_AmbientOcclusion.Depth.Test = DepthTest::Off;

	rp_Composite.pShader = CompositeShader.get();
	rp_Composite.Depth.Test = DepthTest::Off;

	rp_Blit.pShader = BlitShader.get();
	rp_Blit.Depth.Test = DepthTest::Off;

	rp_DebugGeometry.Depth.Test = DepthTest::Off;
	rp_DebugGeometry.pShader = DefaultMeshShader.get();
	rp_DebugGeometry.CullFace = Face::Front;
	rp_DebugGeometry.Blend.Enable = true;

	rp_ScreenspaceUI.Depth.Test = DepthTest::Off;
	rp_ScreenspaceUI.Blend.Enable = true;
	rp_ScreenspaceUI.CullFace = Face::Back;
}

static void load_models()
{
	auto load = [](VoxelEntity& obj, auto path, Float3 pos)
	{
		obj.Mesh = VoxelMesh::load_from_file(path);
		obj.Transform.Position = pos;
		obj.Transform.Scale = (Float3)obj.Mesh.m_Texture->get_dimensions() * 0.1f;
	};

	load(gas_tank, "resources/models/gas_tank_22.png", {});
	load(blue_car, "resources/models/blue_car_13.png", { -4.0f, 0.0f, -2.0f });
	load(box, "resources/models/box_120.png", {});
	load(transformer, "resources/models/transformer_57.png", { -5.0f, 1.05f, -4.05f });
	load(miniPole, "resources/models/pole_12.png", { -2.5f, 0.0f, -1.8f });
	load(chunk, "resources/models/chunk_80.png", { 2.0f, 0.0f, 4.0f });
}

void testbed_start(App& app)
{
	Window* window = app.get_window();

	cameraController.m_Camera = &camera;
	cameraController.m_TargetPosition = { -3.0f, 0.0f, 0.0f };
	cameraController.m_TargetEuler = { 0.0f, -90.0f, 0.0f };

	SceneRenderer::init(window->get_width(), window->get_height());
	reload_all_shaders();
	init_renderpass();
	load_models();

	blueNoise = Texture2D::load("resources/textures/blue_noise_512.png");
	blueNoise->set_wrap_mode(TextureWrapMode::Repeat);
	blueNoise->set_filter_mode(TextureFilterMode::Point);
	
	crosshair = Texture2D::load("resources/textures/crosshair0.png");
	crosshair->set_wrap_mode(TextureWrapMode::Clamp);
	crosshair->set_filter_mode(TextureFilterMode::Linear);

	s_ComputeImageTarget = Texture2D::create(window->get_width(), window->get_height(), TextureFormat::RGBA8);

	s_Font = Font::load_from_file("resources/fonts/Raleway-Regular.ttf");
}

void testbed_update(App& app)
{
	Window* window = app.get_window();
	float deltaTime = app.get_delta_time();
	float elapsedTime = app.get_elapsed_time();
	uint32_t frameNumber = (uint32_t)app.get_frame();

	cameraController.update(deltaTime);

	Matrix4 view = cameraController.get_view();
	Matrix4 projection = camera.get_projection();
	pipeline.begin(view, projection);

	SceneRenderer::begin_frame(camera, cameraController.get_transform());
	auto& framebuffer = SceneRenderer::s_Framebuffer;

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
	pipeline.submit_pass(rp_Geometry, [&]()
	{
		rp_Geometry.pShader->set_float3("u_CameraPosition", cameraController.get_transform().Position);

		SceneRenderer::draw_entities(ENTITIES_TO_RENDER);
	});

	static Float3 lightPos = { -1.0f, 6.0f, 1.0f };
	// LIGHTING SHT
	//if (false)
	{
		static float radius = 8.0f;

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
		pipeline.submit_pass(rp_Stencil, [&]()
		{
			Graphics::reset_draw_buffers();

			Shader_NoFragment->set_matrix("u_Transformation", lightTransformation);

			Graphics::draw_sphere(); // lighting volume mesh
		});

		framebuffer->bind(); // fix draw buffers

		// LIGHT PASS
		pipeline.submit_pass(rp_Lighting, [&]()
		{ 
			Graphics::set_blend_function(1, 1); // GL_ONE = 1 = mindblown

			// Matrices
			LightShader->set_matrix("u_Transformation", lightTransformation);
			LightShader->set_matrix("u_InverseView", glm::inverse(view));
			LightShader->set_matrix("u_InverseProjection", glm::inverse(projection));

			LightShader->set_float("u_LightIntensity", intensity);
			LightShader->set_float("u_LightRadius", radius);

			Float3 color = { 0.9f, 0.9f, 0.05f };
			LightShader->set_float3("u_Color", color);
			LightShader->set_float3("u_VolumeCenter", lightPos);
			LightShader->set_float2("u_ViewportDims", { window->get_width(), window->get_height() });
			LightShader->set_int("u_FrameNumber", frameNumber);
			framebuffer->m_DepthStencilAttachment->bind(0);
			framebuffer->m_ColorAttachments[1]->bind(2);

			Graphics::draw_sphere(); // lighting volume mesh
		});
	}

	Graphics::set_blend_function(0x0302, 0x0303);

	Texture2D* ao_read_texture = nullptr, * ao_write_texture = nullptr;
	// AO PASS
	pipeline.submit_pass(rp_AmbientOcclusion, [&]()
	{
		// HANDLE AO PING PONG
		{
			uint32_t readIndex = frameNumber % 2;
			uint32_t writeIndex = 1 - readIndex;

			const uint32_t aoIndex = 3;
			uint32_t fbo = framebuffer->get_handle();
			const auto& read = framebuffer->m_ColorAttachments[aoIndex + readIndex];
			const auto& write = framebuffer->m_ColorAttachments[aoIndex + writeIndex];
			framebuffer->set_index_handle(3, write->get_handle());

			ao_read_texture = read.get();
			ao_write_texture = write.get();
			read->bind(6);
		}

		framebuffer->m_DepthStencilAttachment->bind(0);
		SceneRenderer::get_shadow_map()->bind(1);
		framebuffer->m_ColorAttachments[5]->bind(2);

		framebuffer->m_ColorAttachments[0]->bind(3);
		framebuffer->m_ColorAttachments[1]->bind(4);
		framebuffer->m_ColorAttachments[2]->bind(5);
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
	pipeline.submit_pass(rp_Blit, [&]()
	{
		framebuffer->m_DepthStencilAttachment->bind(0);
		framebuffer->m_ColorAttachments[1]->bind(1);
		Graphics::draw_fullscreen_triangle(BlitShader);
	});

	Float2 viewport = { (float)window->get_width(), (float)window->get_height() };

	// COMPUTE
	{
		s_ComputeImageTarget->bind_as_image(0, TextureAccessMode::Write);
		uint32_t localSizeX = 16, localSizeY = 16;
		s_Compute->dispatch(
			(viewport.x + localSizeX - 1) / localSizeX,
			(viewport.y + localSizeY - 1) / localSizeY
		);
	}

	// READS AO ACCUM
	pipeline.submit_pass(rp_Composite, [&]()
	{
		Framebuffer::unbind();
		framebuffer->m_ColorAttachments[0]->bind(0);
		framebuffer->m_ColorAttachments[1]->bind(1);
		ao_write_texture->bind(2);
		framebuffer->m_ColorAttachments[6]->bind(3);
		s_ComputeImageTarget->bind(4);

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
	return;

	pipeline.submit_pass(rp_DebugGeometry, [&]()
	{
		Framebuffer::unbind();

		Float3 center = Float3(viewRay.cell) * 0.1f + 0.05f;
		Float3 scale = Float3(0.1f) + 0.001f;

		Matrix4 debuglighttransform = Transformation(center, {}, scale).get_transform();
		DefaultMeshShader->set_float4("u_Color", { 1.0f, 1.0f, 1.0f, 0.4f });
		DefaultMeshShader->set_matrix("u_Transformation", debuglighttransform);
		DefaultMeshShader->set_float2("u_ViewportDims", viewport);
		framebuffer->m_DepthStencilAttachment->bind(0);

		Graphics::draw_cube();
	});

	Matrix4 pixelProjection = glm::ortho(0.0f, viewport.x, 0.0f, viewport.y, -1.0f, 1.0f);
	pipeline.submit_pass(rp_ScreenspaceUI, [&]()
	{
		// debug text
		s_TextShader->bind();
		s_TextShader->set_matrix("u_ViewProjection", pixelProjection);
		{
			s_TextShader->set_matrix("u_Transformation",
				Transformation({ 25.0f, viewport.y - 80.0f, 0.0f }, {}, { 40.0f, 40.0f, 1.0f }).get_transform());
			Graphics::draw_text(std::format("ms: {:.3f}", deltaTime * 1000.0f), s_Font);
		}
		{
			s_TextShader->set_matrix("u_Transformation",
				Transformation({ 25.0f, viewport.y - 160.0f, 0.0f }, {}, { 40.0f, 40.0f, 1.0f }).get_transform());
			Graphics::draw_text(std::format("fps: {:.2f}", 1.0f / deltaTime), s_Font);
		}
		{
			Float3 cam = (cameraController.get_transform().Position * 0.001f) * 1000.0f;
			s_TextShader->set_matrix("u_Transformation",
				Transformation({ 25.0f, viewport.y - 240.0f, 0.0f }, {}, { 40.0f, 40.0f, 1.0f }).get_transform());
			Graphics::draw_text(std::format("({:.2f}, {:.2f}, {:.2f})", cam.x, cam.y, cam.z), s_Font);
		}

		// crosshair idfk
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
}

void testbed_stop(App& app)
{

}

void testbed_window_resized(WindowResizeEvent& e)
{
	SceneRenderer::s_Framebuffer->resize(e.width, e.height);
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