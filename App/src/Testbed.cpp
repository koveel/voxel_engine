#include "pch.h"
#include "App.h"
#include "utils/CameraController.h"

#include "rendering/Shader.h"
#include "rendering/scene/SceneRenderer.h"
#include "rendering/RenderPipeline.h"

#include "windowing/Window.h"

#include "voxel/Terrain.h"

#include "gui/Font.h"
 
using namespace Engine;

static Camera camera = Camera(ProjectionType::Perspective);
static CameraController cameraController;

static owning_ptr<Framebuffer> s_Framebuffer;

static RenderPipeline renderPipeline;
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
static owning_ptr<Shader> SpriteShader;
static owning_ptr<Shader> TextShader;

static owning_ptr<ComputeShader> ComputeAOShader;
static owning_ptr<ComputeShader> ComputeSSAOShader;

static owning_ptr<Font> s_Font;

static VoxelEntity gas_tank;
static VoxelEntity blue_car;
static VoxelEntity box;
static VoxelEntity transformer;
static VoxelEntity miniPole;
static VoxelEntity chunk;

static owning_ptr<Texture2D> blueNoise;
static owning_ptr<Texture2D> crosshair;
static owning_ptr<Texture2D> testTexture;

static VoxelEntity simplex_chunk;

static Matrix4 s_PreviousViewProjection = Matrix4(1.0f);

static const std::vector<VoxelEntity*> ENTITIES_TO_RENDER = {
	&simplex_chunk
};

static Terrain::GenerationParameters s_TerrainParams =
{
	.noise = {
		.amplitude = 2.0f,
		.frequency = 0.5f,
		.lacunarity = 2.0f,
		.persistence = 0.7f,
		.octaves = 4
	},
	.width = 8192,
	.height = 16,
};

static void generate_terrain()
{
	auto& noise = s_TerrainParams.noise;
	noise.amplitude = glm::clamp(noise.amplitude, 0.1f, 2.0f);
	noise.frequency = glm::clamp(noise.frequency, 0.1f, 2.0f);
	noise.lacunarity = glm::clamp(noise.lacunarity, 0.1f, 4.0f);
	noise.persistence = glm::clamp(noise.persistence, 0.1f, 4.0f);
	noise.octaves = glm::clamp(noise.octaves, 1u, 6u);

	simplex_chunk.Mesh = Terrain::generate_terrain(s_TerrainParams);
	simplex_chunk.Transform.Scale = (Float3)simplex_chunk.Mesh.m_Texture->get_dimensions() * 0.1f;
}

static void create_framebuffer(uint32_t screen_width, uint32_t screen_height)
{
	// Create framebuffer
	FramebufferDescriptor descriptor;
	descriptor.ColorAttachments = {
		{ screen_width, screen_height, TextureFormat::RGBA8 }, // albedo
		{ screen_width, screen_height, TextureFormat::RGB8S }, // normal

		{ screen_width, screen_height, TextureFormat::RGB8S, false }, // normal last frame
		{ screen_width, screen_height, TextureFormat::R8, false }, // AO accumulation tex 1
		{ screen_width, screen_height, TextureFormat::R8, false }, // AO accumulation tex 2
		{ screen_width, screen_height, TextureFormat::R16, false }, // depth last frame

		{ screen_width, screen_height, TextureFormat::RGBA8 }, // lighting
	};
	FramebufferDescriptor::DepthStencilAttachment depthStencilAttachment = {
		screen_width, screen_height, 24, true
	};
	descriptor.pDepthStencilAttachment = &depthStencilAttachment;

	s_Framebuffer = Framebuffer::create(descriptor);

	// clear taa
	float v = 1.0f;
	s_Framebuffer->m_ColorAttachments[3]->clear_to(&v);
	s_Framebuffer->m_ColorAttachments[4]->clear_to(&v);
}

static void reload_all_shaders()
{
	LightShader = Shader::create("resources/shaders/LightShader.glsl");
	rp_Lighting.pShader = LightShader.get();
	DefaultMeshShader = Shader::create("resources/shaders/DefaultShader.glsl");
	rp_DebugGeometry.pShader = DefaultMeshShader.get();
	SceneRenderer::s_VoxelMeshShader = Shader::create("resources/shaders/VoxelShader.glsl");
	rp_Geometry.pShader = SceneRenderer::s_VoxelMeshShader.get();
	Shader_NoFragment = Shader::create("resources/shaders/VertexOnlyShader.glsl");
	rp_Stencil.pShader = Shader_NoFragment.get();
	CompositeShader = Shader::create("resources/shaders/CompositeShader.glsl");
	rp_Composite.pShader = CompositeShader.get();
	BlitShader = Shader::create("resources/shaders/BlitShader.glsl");
	rp_Blit.pShader = BlitShader.get();
	SpriteShader = Shader::create("resources/shaders/SpriteShader.glsl");
	TextShader = Shader::create("resources/shaders/TextShader.glsl");

	ComputeAOShader = ComputeShader::create("resources/shaders/compute/ComputeAO.glsl");
	ComputeSSAOShader = ComputeShader::create("resources/shaders/compute/ComputeSSAO.glsl");
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
	generate_terrain();
	return;

	load(blue_car, "resources/models/blue_car_13.png", { -4.0f, 0.0f, -2.0f });
	load(box, "resources/models/box_120.png", {});
	load(transformer, "resources/models/transformer_57.png", { -5.0f, 3.05f, 0.05f });
	load(miniPole, "resources/models/pole_12.png", { -2.5f, 0.0f, -1.8f });
	load(chunk, "resources/models/chunk_80.png", { 0.0f, 0.0f, 0.0f });
}

void testbed_start(App& app)
{
	Window* window = app.get_window();

	cameraController.m_Camera = &camera;
	cameraController.m_TargetPosition = { -3.0f, 4.0f, 0.0f };
	cameraController.m_TargetEuler = { 0.0f, -90.0f, 0.0f };

	create_framebuffer(window->get_width(), window->get_height());
	reload_all_shaders();
	init_renderpass();
	load_models();

	blueNoise = Texture2D::load("resources/textures/blue_noise_512.png");
	blueNoise->set_wrap_mode(TextureWrapMode::Repeat);
	blueNoise->set_filter_mode(TextureFilterMode::Point);
	
	crosshair = Texture2D::load("resources/textures/crosshair0.png");
	crosshair->set_wrap_mode(TextureWrapMode::Clamp);
	crosshair->set_filter_mode(TextureFilterMode::Linear);

	testTexture = Texture2D::load("resources/textures/plastic_normals.jpg");
	testTexture->set_wrap_mode(TextureWrapMode::Repeat);
	testTexture->set_filter_mode(TextureFilterMode::Linear);

	s_Font = Font::load_from_file("resources/fonts/Raleway-Regular.ttf");
}

static TracedRay sceneCameraRay;

static void handle_scene_view_ray_trace(const Transformation& camera)
{
	const float MaxDistanceMeters = 10.0f;

	Float3 origin = camera.Position;
	Float3 euler = camera.Rotation;

	float pitch = glm::radians(euler.x), yaw = glm::radians(euler.y);
	Float3 direction = Float3(
		glm::cos(pitch) * -glm::sin(yaw),
		glm::sin(pitch),
		glm::cos(pitch) * -glm::cos(yaw)
	);

	sceneCameraRay = SceneRenderer::trace_ray_through_scene(origin, direction, MaxDistanceMeters);
}

static Float3 get_highlighted_voxel_center()
{
	Float3 cell = Float3(sceneCameraRay.cell);
	Float3 center = cell * 0.1f + 0.05f;
	return center;
}

void testbed_update(App& app)
{
	Window* window = app.get_window();
	Float2 viewport = { (float)window->get_width(), (float)window->get_height() };

	float deltaTime = app.get_delta_time();
	float elapsedTime = app.get_elapsed_time();
	uint32_t frameNumber = (uint32_t)app.get_frame();

	cameraController.update(deltaTime);

	Matrix4 view = cameraController.get_view();
	Matrix4 projection = camera.get_projection();
	renderPipeline.begin(view, projection);

	s_Framebuffer->bind();
	s_Framebuffer->clear({ 0.0f });

	SceneRenderer::begin_frame(camera, cameraController.get_transform());

	if (Input::was_key_pressed(Key::R) && Input::is_key_down(Key::Control))
		reload_all_shaders();

	handle_scene_view_ray_trace(cameraController.get_transform());

	auto last = s_TerrainParams;
	if (Input::was_key_pressed(Key::Y))
		s_TerrainParams.noise.amplitude += 0.1f * Input::is_key_down(Key::Tab) ? -1.0f : 1.0f;
	if (Input::was_key_pressed(Key::U))
		s_TerrainParams.noise.frequency += 0.1f * (Input::is_key_down(Key::Tab) ? -1.0f : 1.0f);
	if (Input::was_key_pressed(Key::I))
		s_TerrainParams.noise.lacunarity += 0.1f * Input::is_key_down(Key::Tab) ? -1.0f : 1.0f;
	if (Input::was_key_pressed(Key::O)) 
		s_TerrainParams.noise.persistence += 0.1f * (Input::is_key_down(Key::Tab) ? -1.0f : 1.0f);
	if (Input::was_key_pressed(Key::P))
		s_TerrainParams.noise.octaves += Input::is_key_down(Key::Tab) ? -1 : 1;

	if (Input::was_key_pressed(Key::P))
	{
		LOG(s_TerrainParams.noise.amplitude);
		LOG(s_TerrainParams.noise.frequency);
		LOG(s_TerrainParams.noise.lacunarity);
		LOG(s_TerrainParams.noise.persistence);
		LOG(s_TerrainParams.noise.octaves);
	}

	if (memcmp(&s_TerrainParams, &last, sizeof(s_TerrainParams)) != 0)
	{
		generate_terrain();
	}

	// GEOMETRY PASS
	renderPipeline.submit_pass(rp_Geometry, [&]()
	{
		testTexture->bind(2);

		static uint32_t tile = 4;
		if (Input::was_key_pressed(Key::UpArrow))
			tile++;
		if (Input::was_key_pressed(Key::DownArrow))
			tile--;

		rp_Geometry.pShader->set_float3("u_CameraPosition", cameraController.get_transform().Position);
		rp_Geometry.pShader->set_int("u_TextureTileFactor", tile);

		if (Input::is_key_down(Key::M))
			SceneRenderer::draw_shadow_map();
		else
			SceneRenderer::draw_entities(ENTITIES_TO_RENDER);
	});

	static Float3 lightPos = { -3.0f, 5.0f, 2.0f };
	if (false)
	// LIGHTING SHT
	{
		static float radius = 8.0f; 

		auto lightTransformation = Transformation(lightPos, Float3(0.0f), Float3(radius * 2, radius * 2, radius * 2)).get_transform();
		static float intensity = 1.0f;

		// STENCIL PASS
		renderPipeline.submit_pass(rp_Stencil, [&]()
		{
			Graphics::reset_draw_buffers();

			Shader_NoFragment->set_matrix("u_Transformation", lightTransformation);

			Graphics::draw_sphere(); // lighting volume mesh
		});

		s_Framebuffer->bind(); // fix draw buffers

		// LIGHT PASS
		renderPipeline.submit_pass(rp_Lighting, [&]()
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
			s_Framebuffer->m_DepthStencilAttachment->bind(0);
			s_Framebuffer->m_ColorAttachments[1]->bind(2);
			SceneRenderer::get_shadow_map()->bind(1);
			blueNoise->bind(9);

			Graphics::draw_sphere(); // lighting volume mesh
		});
	}

	bool ssao = false;
	Texture2D* fresh_ao_texture = s_Framebuffer->m_ColorAttachments[3].get(); // For final composite
	// COMPUTE AO
	if (!ssao)
	{
		ComputeAOShader->bind();

		SceneRenderer::get_shadow_map()->bind(0);
		blueNoise->bind(1);
		s_Framebuffer->m_DepthStencilAttachment->bind(2);
		s_Framebuffer->m_ColorAttachments[5]->bind(3);
		s_Framebuffer->m_ColorAttachments[1]->bind(4);
		s_Framebuffer->m_ColorAttachments[2]->bind(5);

		// ao accum tex ping pong
		{
			uint32_t readIndex = frameNumber % 2;
			uint32_t writeIndex = 1 - readIndex;

			const uint32_t aoIndex = 3;
			uint32_t fbo = s_Framebuffer->get_handle();
			const auto& read = s_Framebuffer->m_ColorAttachments[aoIndex + readIndex];
			const auto& write = s_Framebuffer->m_ColorAttachments[aoIndex + writeIndex];
			
			read->bind(6);
			write->bind_as_image(0, TextureAccessMode::Write);

			fresh_ao_texture = write.get();
		}

		ComputeAOShader->set_matrix("u_InverseView", glm::inverse(view));
		ComputeAOShader->set_matrix("u_InverseProjection", glm::inverse(projection));
		ComputeAOShader->set_matrix("u_PrevFrameViewProjection", s_PreviousViewProjection);
		ComputeAOShader->set_matrix("u_ViewProjection", projection * view);
		ComputeAOShader->set_int("u_FrameNumber", frameNumber);

		uint32_t localSizeX = 16, localSizeY = 16;
		ComputeAOShader->dispatch(
			(viewport.x + localSizeX - 1) / localSizeX,
			(viewport.y + localSizeY - 1) / localSizeY
		);
	}
	
	if (ssao)
	// COMPUTE SSAO
	{
		ComputeSSAOShader->bind();

		s_Framebuffer->m_DepthStencilAttachment->bind(0);
		s_Framebuffer->m_ColorAttachments[1]->bind(1);

		// ao accum tex ping pong
		{
			uint32_t readIndex = frameNumber % 2;
			uint32_t writeIndex = 1 - readIndex;

			const uint32_t aoIndex = 3;
			uint32_t fbo = s_Framebuffer->get_handle();
			const auto& read = s_Framebuffer->m_ColorAttachments[aoIndex + readIndex];
			const auto& write = s_Framebuffer->m_ColorAttachments[aoIndex + writeIndex];

			read->bind(2);
			write->bind_as_image(0, TextureAccessMode::Write);

			fresh_ao_texture = write.get();
		}

		ComputeSSAOShader->set_matrix("u_InverseView", glm::inverse(view));
		ComputeSSAOShader->set_matrix("u_InverseProjection", glm::inverse(projection));
		ComputeSSAOShader->set_matrix("u_ViewProjection", projection * view);

		uint32_t localSizeX = 16, localSizeY = 16;
		ComputeSSAOShader->dispatch(
			(viewport.x + localSizeX - 1) / localSizeX,
			(viewport.y + localSizeY - 1) / localSizeY
		);
	}

	// Blit depth and normals into old depth / old normal
	renderPipeline.submit_pass(rp_Blit, [&]()
	{
		s_Framebuffer->m_DepthStencilAttachment->bind(0);
		s_Framebuffer->m_ColorAttachments[1]->bind(1);
		Graphics::draw_fullscreen_triangle(BlitShader);
	});

	Framebuffer::unbind();

	// READS AO ACCUM
	renderPipeline.submit_pass(rp_Composite, [&]()
	{
		s_Framebuffer->m_ColorAttachments[0]->bind(0);
		s_Framebuffer->m_ColorAttachments[1]->bind(1);
		s_Framebuffer->m_ColorAttachments[6]->bind(3);
		fresh_ao_texture->bind(2);

		Graphics::draw_fullscreen_triangle(CompositeShader);
	});

	Graphics::set_blend_function(0x0302, 0x0303);
	renderPipeline.submit_pass(rp_DebugGeometry, [&]()
	{
		DefaultMeshShader->set_float2("u_ViewportDims", viewport);
		DefaultMeshShader->set_int("u_DepthClip", false);

		static bool render_outline = false;

		if (Input::was_key_pressed(Key::B))
			render_outline = !render_outline;

		Graphics::set_draw_mode(PrimitiveMode::LineStrip);
		// Bounding boxes
		if (render_outline)
		for (VoxelEntity* pentity : ENTITIES_TO_RENDER)
		{
			Transformation t = pentity->Transform;
			auto transformation = t.get_transform();

			DefaultMeshShader->set_float4("u_Color", { 0.9f, 0.9f, 0.1f, 0.4f });
			DefaultMeshShader->set_matrix("u_Transformation", transformation);
			Graphics::draw_cube();
		}
		Graphics::set_draw_mode(PrimitiveMode::Triangles);

		// voxel cursor highlight
		Float3 center = get_highlighted_voxel_center();
		Float3 scale = Float3(0.1f) + 0.001f;
		Matrix4 highlight_transform = Transformation(center, {}, scale).get_transform();
		DefaultMeshShader->set_float4("u_Color", { 1.0f, 1.0f, 1.0f, 0.4f });
		DefaultMeshShader->set_matrix("u_Transformation", highlight_transform);
		DefaultMeshShader->set_int("u_DepthClip", true);
		s_Framebuffer->m_DepthStencilAttachment->bind(0);

		Graphics::draw_cube();
	});

	Matrix4 pixelProjection = glm::ortho(0.0f, viewport.x, 0.0f, viewport.y, -1.0f, 1.0f);
	renderPipeline.submit_pass(rp_ScreenspaceUI, [&]()
	{
		// debug text
		TextShader->bind();
		TextShader->set_matrix("u_ViewProjection", pixelProjection);
		{
			TextShader->set_matrix("u_Transformation",
				Transformation({ 25.0f, viewport.y - 80.0f, 0.0f }, {}, { 40.0f, 40.0f, 1.0f }).get_transform());
			Graphics::draw_text(std::format("ms: {:.3f}", deltaTime * 1000.0f), s_Font);
		}
		{
			TextShader->set_matrix("u_Transformation",
				Transformation({ 25.0f, viewport.y - 160.0f, 0.0f }, {}, { 40.0f, 40.0f, 1.0f }).get_transform());
			Graphics::draw_text(std::format("fps: {:.2f}", 1.0f / deltaTime), s_Font);
		}
		// Camera position
		{
			Float3 cam = (cameraController.get_transform().Position * 0.001f) * 1000.0f;
			TextShader->set_matrix("u_Transformation",
				Transformation({ 25.0f, viewport.y - 240.0f, 0.0f }, {}, { 40.0f, 40.0f, 1.0f }).get_transform());
			Graphics::draw_text(std::format("({:.2f}, {:.2f}, {:.2f})", cam.x, cam.y, cam.z), s_Font);
		}
		// Highlighted cell
		{
			Float3 cell = get_highlighted_voxel_center() - Float3(0.05f);
			TextShader->set_matrix("u_Transformation",
				Transformation({ 25.0f, viewport.y - 300.0f, 0.0f }, {}, { 40.0f, 40.0f, 1.0f }).get_transform());
			Graphics::draw_text(std::format("({:.2f}, {:.2f}, {:.2f})", cell.x, cell.y, cell.z), s_Font);
		}

		// crosshair idfk
		SpriteShader->bind();
		SpriteShader->set_float4("u_Tint", { 0.0f, 0.0f, 0.0f, 1.0f });
		crosshair->bind(0);
		SpriteShader->set_matrix("u_ViewProjection", pixelProjection);

		float aspect = viewport.y / viewport.x;
		Matrix4 transform = Transformation({ viewport.x / 2.0f, viewport.y / 2.0f, 0.0f }, {},
			{ 100.0f, 100.0f, 1.0f }).get_transform();
		SpriteShader->set_matrix("u_Transformation", transform);
		Graphics::draw_quad();
	});	

	s_PreviousViewProjection = renderPipeline.m_ViewProjectionMatrix;
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