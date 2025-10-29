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
static owning_ptr<Shader> DepthAndNormal_BlitShader;
static owning_ptr<Shader> Shader_NoFragment;
static owning_ptr<Shader> SpriteShader;
static owning_ptr<Shader> TextShader;

static owning_ptr<ComputeShader> ComputeAOShader;
static owning_ptr<ComputeShader> Compute_BlitShader;

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

static owning_ptr<TerrainGenerator> s_TerrainGen;

static Matrix4 s_PreviousViewProjection = Matrix4(1.0f);

static void create_framebuffer(uint32_t screen_width, uint32_t screen_height)
{
	// Create framebuffer
	// TODO: revisit - some of these may be unneccessary??
	FramebufferDescriptor descriptor;
	descriptor.ColorAttachments = {
		{ screen_width, screen_height, TextureFormat::RGBA8 }, // albedo
		{ screen_width, screen_height, TextureFormat::RGB8S }, // normal

		{ screen_width, screen_height, TextureFormat::RGB8S, false }, // normal last frame
		{ screen_width, screen_height, TextureFormat::R8, false }, // AO accumulation tex 1
		{ screen_width, screen_height, TextureFormat::R8, false }, // AO accumulation tex 2
		{ screen_width, screen_height, TextureFormat::R32F, false }, // depth last frame

		{ screen_width, screen_height, TextureFormat::RGBA8 }, // lighting
		{ screen_width, screen_height, TextureFormat::R32F }, // depth occlusion
	};
	FramebufferDescriptor::DepthStencilAttachment depthStencilAttachment = {
		screen_width, screen_height, TextureFormat::Depth32FStencil8
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
	DepthAndNormal_BlitShader = Shader::create("resources/shaders/BlitShader.glsl");
	rp_Blit.pShader = DepthAndNormal_BlitShader.get();
	SpriteShader = Shader::create("resources/shaders/SpriteShader.glsl");
	TextShader = Shader::create("resources/shaders/TextShader.glsl");

	ComputeAOShader = ComputeShader::create("resources/shaders/compute/ComputeAO.glsl");
	Compute_BlitShader = ComputeShader::create("resources/shaders/compute/Compute_Blit.glsl");
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

	rp_Blit.pShader = DepthAndNormal_BlitShader.get();
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
	load(transformer, "resources/models/transformer_57.png", { -5.0f, 3.05f, 0.05f });
	load(miniPole, "resources/models/pole_12.png", { -2.5f, 0.0f, -1.8f });
	load(chunk, "resources/models/chunk_80.png", { 0.0f, 0.0f, 0.0f });
}

void testbed_start(App& app)
{
	Window* window = app.get_window();

	cameraController.m_Camera = &camera;
	cameraController.m_TargetPosition = { 2.0f, 40.0f, 50.0f };
	cameraController.m_TargetEuler = { -30.0f, 0.0f, 0.0f };

	create_framebuffer(window->get_width(), window->get_height());
	reload_all_shaders();
	init_renderpass();

	s_TerrainGen = make_owning<TerrainGenerator>();

#define NUM_CHUNKS 128

#if NUM_CHUNKS == 1
	s_TerrainGen->generate_chunk({});
#else
	int dim = glm::sqrt(NUM_CHUNKS);
	for (int x = 0; x < dim; x++)
	for (int y = 0; y < dim; y++)
	{
		Int2 idx = { x - dim / 2, y - dim / 2 };
		s_TerrainGen->generate_chunk(idx);
	}
#endif
	s_TerrainGen->generate_shadowmap({});

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

static void draw_shadow_map(uint32_t level)
{
	auto& texture = s_TerrainGen->m_ShadowMap;
	Int3 voxelDimensions = texture->get_dimensions();

	Matrix4 rotation = Matrix4(1.0f);
	Matrix4 transformation = glm::scale(glm::mat4(1.0f), (Float3)voxelDimensions * VoxelScaleMeters);

	auto& shader = SceneRenderer::s_VoxelMeshShader;
	shader->bind();
	shader->set("u_OBBCenter", Float3(0.0f, 0.0f, 0.0f));
	shader->set("u_OBBOrientation", glm::inverse(rotation));
	shader->set("u_VoxelDimensions", voxelDimensions);
	shader->set("u_MaterialIndex", 1);
	shader->set("u_MipLevel", level);

	shader->set("u_Transformation", transformation);

	texture->bind();
	VoxelMesh::s_MaterialPalette->bind(3);
	Graphics::draw_cube();
	shader->set("u_MipLevel", 0);
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

	// hot reload
	if (Input::was_key_pressed(Key::R) && Input::is_key_down(Key::Control))
	{
		reload_all_shaders();
		s_TerrainGen->m_TerrainShader = Shader::create("resources/shaders/TerrainShader.glsl");
		s_TerrainGen->generate_shadowmap({});
	}
	handle_scene_view_ray_trace(cameraController.get_transform());

	// GEOMETRY PASS
	renderPipeline.submit_pass(rp_Geometry, [&]()
	{
		s_TerrainGen->m_ShadowMap->bind(1);
		rp_Geometry.pShader->set("u_CameraPosition", cameraController.get_transform().Position);

		static uint32_t tile = 4;
		rp_Geometry.pShader->set("u_TextureTileFactor", tile);

		static int32_t level = 0;
		if (Input::was_key_pressed(Key::UpArrow))
			level++;
		if (Input::was_key_pressed(Key::DownArrow))
			level--;
		level = glm::clamp(level, 0, 2);

		static bool draw_sm = false;
		if (Input::was_key_pressed(Key::M))
			draw_sm = !draw_sm;
		if (draw_sm)
		{
			draw_shadow_map(level);
		}
		else if (!draw_sm)
		{
			if (frameNumber == 0)
				s_TerrainGen->resort_chunks({});

			//float d = 0.0f;
			//auto& occlusion = s_Framebuffer->m_ColorAttachments[7];
			//occlusion->clear_to(&d);

			s_TerrainGen->m_TerrainShader->set("u_MipLevel", level);
			s_TerrainGen->render_terrain(projection * view, cameraController.get_transform().Position);

			//for (TerrainChunk* chunk : s_TerrainGen->m_SortedChunks)
			//{
			//	occlusion->bind(2);
			//
			//	SceneRenderer::draw_mesh(chunk->mesh, chunk->position, {});
			//
			//	Graphics::memory_barrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
			//
			//	Compute_BlitShader->bind();
			//	s_Framebuffer->m_DepthStencilAttachment->bind(0);
			//	occlusion->bind_as_image(1, TextureAccessMode::Write);
			//
			//	uint32_t localSizeX = 16, localSizeY = 16;
			//	Compute_BlitShader->dispatch(
			//		(viewport.x + localSizeX - 1) / localSizeX,
			//		(viewport.y + localSizeY - 1) / localSizeY
			//	);
			//	Graphics::memory_barrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			//}
		}
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

			Shader_NoFragment->set("u_Transformation", lightTransformation);

			Graphics::draw_sphere(); // lighting volume mesh
		});

		s_Framebuffer->bind(); // fix draw buffers

		// LIGHT PASS
		renderPipeline.submit_pass(rp_Lighting, [&]()
		{ 
			Graphics::set_blend_function(1, 1); // GL_ONE = 1 = mindblown

			// Matrices
			LightShader->set("u_Transformation", lightTransformation);
			LightShader->set("u_InverseView", glm::inverse(view));
			LightShader->set("u_InverseProjection", glm::inverse(projection));

			LightShader->set("u_LightIntensity", intensity);
			LightShader->set("u_LightRadius", radius);

			Float3 color = { 0.9f, 0.9f, 0.05f };
			LightShader->set("u_Color", color);
			LightShader->set("u_VolumeCenter", lightPos);
			LightShader->set("u_ViewportDims", Float2(window->get_width(), window->get_height()));
			LightShader->set("u_FrameNumber", frameNumber);
			s_Framebuffer->m_DepthStencilAttachment->bind(0);
			s_Framebuffer->m_ColorAttachments[1]->bind(2);
			s_TerrainGen->m_ShadowMap->bind(1);
			blueNoise->bind(9);

			Graphics::draw_sphere(); // lighting volume mesh
		});
	}

	bool ssao = false;
	Texture2D* fresh_ao_texture = s_Framebuffer->m_ColorAttachments[3].get(); // For final composite
	
	bool show_ao = !Input::is_key_down(Key::A_9);
	if (!show_ao)
	{
		float v = 1.0f;
		s_Framebuffer->m_ColorAttachments[3]->clear_to(&v);
		s_Framebuffer->m_ColorAttachments[4]->clear_to(&v);
	}

	// COMPUTE AO
	if (!ssao && show_ao)
	{
		ComputeAOShader->bind();

		s_TerrainGen->m_ShadowMap->bind(0);
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

		ComputeAOShader->set("u_InverseView", glm::inverse(view));
		ComputeAOShader->set("u_InverseProjection", glm::inverse(projection));
		ComputeAOShader->set("u_PrevFrameViewProjection", s_PreviousViewProjection);
		ComputeAOShader->set("u_ViewProjection", projection * view);
		ComputeAOShader->set("u_FrameNumber", frameNumber);

		ComputeAOShader->set("u_CameraPos", cameraController.m_Transformation.Position);

		uint32_t localSizeX = 16, localSizeY = 16;
		ComputeAOShader->dispatch(
			(viewport.x + localSizeX - 1) / localSizeX,
			(viewport.y + localSizeY - 1) / localSizeY
		);
		Graphics::memory_barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
	
	// TODO: computize ?
	// Blit depth and normals into old depth / old normal
	renderPipeline.submit_pass(rp_Blit, [&]()
	{
		s_Framebuffer->m_DepthStencilAttachment->bind(0);
		s_Framebuffer->m_ColorAttachments[1]->bind(1);
		Graphics::draw_fullscreen_triangle(DepthAndNormal_BlitShader);
	});

	Framebuffer::unbind();

	// READS AO ACCUM
	renderPipeline.submit_pass(rp_Composite, [&]()
	{
		s_Framebuffer->m_ColorAttachments[0]->bind(0);
		s_Framebuffer->m_ColorAttachments[1]->bind(1);
		s_Framebuffer->m_ColorAttachments[6]->bind(3);
		s_Framebuffer->m_DepthStencilAttachment->bind(4);
		fresh_ao_texture->bind(2);

		s_Framebuffer->m_ColorAttachments[7]->bind(5);

		static int output = 0;
		if (Input::was_key_pressed(Key::A_1))
			output = 0;
		if (Input::was_key_pressed(Key::A_2))
			output = 1;
		if (Input::was_key_pressed(Key::A_3))
			output = 2;
		if (Input::was_key_pressed(Key::A_4))
			output = 3;
		if (Input::was_key_pressed(Key::A_5))
			output = 4;

		CompositeShader->set("u_Output", output);
		CompositeShader->set("u_InverseView", glm::inverse(view));
		CompositeShader->set("u_InverseProjection", glm::inverse(projection));
		CompositeShader->set("u_CameraPos", cameraController.m_Transformation.Position);

		Graphics::draw_fullscreen_triangle(CompositeShader);
	});

	Graphics::set_blend_function(0x0302, 0x0303);
	renderPipeline.submit_pass(rp_DebugGeometry, [&]()
	{
		DefaultMeshShader->set("u_ViewportDims", viewport);
		DefaultMeshShader->set("u_DepthClip", false);

		static bool render_outline = false;
		if (Input::was_key_pressed(Key::B))
			render_outline = !render_outline;

		Graphics::set_draw_mode(PrimitiveMode::LineStrip);

		if (render_outline)
		{
			for (auto& pair : s_TerrainGen->m_ChunkTable)
			{
				auto& chunk = pair.second;
				Transformation t = { chunk.position, {}, Float3(chunk.mesh.m_Texture->get_dimensions()) * VoxelScaleMeters };
				auto transformation = t.get_transform();

				Float3 colors[] =
				{
					{ 0.9f, 0.2f, 0.1f },
					{ 0.2f, 0.2f, 0.9f },
					{ 0.4f, 0.8f, 0.7f },
					{ 0.1f, 0.9f, 0.2f },
					{ 0.6f, 0.1f, 0.9f },
					{ 0.9f, 0.9f, 0.9f },
				};
				Float3 col = colors[(chunk.index.x + chunk.index.y) % std::size(colors)];
				if (glm::abs(pair.first.x) >= 2 || glm::abs(pair.first.y) >= 2)
					col = { 0.8f, 0.8f, 0.8f };

				DefaultMeshShader->set("u_Color", Float4(col, 1.0f));
				DefaultMeshShader->set("u_Transformation", transformation);
				Graphics::draw_cube();
			}
		}
		Graphics::set_draw_mode(PrimitiveMode::Triangles);

		// voxel cursor highlight
		Float3 center = get_highlighted_voxel_center();
		Float3 scale = Float3(0.1f) + 0.001f;
		Matrix4 highlight_transform = Transformation(center, {}, scale).get_transform();
		DefaultMeshShader->set("u_Color", Float4(1.0f, 1.0f, 1.0f, 0.4f));
		DefaultMeshShader->set("u_Transformation", highlight_transform);
		DefaultMeshShader->set("u_DepthClip", true);
		s_Framebuffer->m_DepthStencilAttachment->bind(0);

		Graphics::draw_cube();
	});

	Matrix4 pixelProjection = glm::ortho(0.0f, viewport.x, 0.0f, viewport.y, -1.0f, 1.0f);
	renderPipeline.submit_pass(rp_ScreenspaceUI, [&]()
	{
		// debug text
		TextShader->bind();
		TextShader->set("u_ViewProjection", pixelProjection);
		{
			TextShader->set("u_Transformation",
				Transformation({ 25.0f, viewport.y - 80.0f, 0.0f }, {}, { 40.0f, 40.0f, 1.0f }).get_transform());
			Graphics::draw_text(std::format("ms: {:.3f}", deltaTime * 1000.0f), s_Font);
		}
		{
			TextShader->set("u_Transformation",
				Transformation({ 25.0f, viewport.y - 160.0f, 0.0f }, {}, { 40.0f, 40.0f, 1.0f }).get_transform());
			Graphics::draw_text(std::format("fps: {:.2f}", 1.0f / deltaTime), s_Font);
		}
		// Camera position
		{
			Float3 cam = (cameraController.get_transform().Position * 0.001f) * 1000.0f;
			TextShader->set("u_Transformation",
				Transformation({ 25.0f, viewport.y - 240.0f, 0.0f }, {}, { 40.0f, 40.0f, 1.0f }).get_transform());
			Graphics::draw_text(std::format("({:.2f}, {:.2f}, {:.2f})", cam.x, cam.y, cam.z), s_Font);
		}
		// Highlighted cell
		{
			Float3 cell = get_highlighted_voxel_center() - Float3(0.05f);
			TextShader->set("u_Transformation",
				Transformation({ 25.0f, viewport.y - 300.0f, 0.0f }, {}, { 40.0f, 40.0f, 1.0f }).get_transform());
			Graphics::draw_text(std::format("({:.2f}, {:.2f}, {:.2f})", cell.x, cell.y, cell.z), s_Font);
		}

		// crosshair idfk
		SpriteShader->bind();
		SpriteShader->set("u_Tint", Float4(0.0f, 0.0f, 0.0f, 1.0f));
		crosshair->bind(0);
		SpriteShader->set("u_ViewProjection", pixelProjection);

		float aspect = viewport.y / viewport.x;
		Matrix4 transform = Transformation({ viewport.x / 2.0f, viewport.y / 2.0f, 0.0f }, {},
			{ 100.0f, 100.0f, 1.0f }).get_transform();
		SpriteShader->set("u_Transformation", transform);
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