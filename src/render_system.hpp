#pragma once

#include <array>
#include <utility>

#include "common.hpp"
#include "font_system.hpp"
#include "tinyECS/components.hpp"
#include "tinyECS/tiny_ecs.hpp"

// System responsible for setting up OpenGL and for rendering all the
// visual entities in the game
class RenderSystem {
	/**
	 * The following arrays store the assets the game will use. They are loaded
	 * at initialization and are assumed to not be modified by the render loop.
	 *
	 * Whenever possible, add to these lists instead of creating dynamic state
	 * it is easier to debug and faster to execute for the computer.
	 */
	std::array<GLuint, texture_count> texture_gl_handles;
	std::array<ivec2, texture_count>  texture_dimensions;

	// Make sure these paths remain in sync with the associated enumerators.
	// Associated id with .obj path
	const std::vector<std::pair<GEOMETRY_BUFFER_ID, std::string>> mesh_paths = {
		std::pair<GEOMETRY_BUFFER_ID, std::string>(GEOMETRY_BUFFER_ID::CHICKEN, mesh_path("chicken.obj"))
		// specify meshes of other assets here
	};

	// Make sure these paths remain in sync with the associated enumerators (see TEXTURE_ASSET_ID).
	const std::array<std::string, texture_count> texture_paths = {
		textures_path("invaders/blue_1.png"),
		textures_path("towers/tower01.png"),
		textures_path("projectiles/gold_bubble.png"),
		textures_path("effects/explosion1.png"),
		textures_path("effects/explosion2.png"),
		textures_path("effects/explosion3.png"),

		/* A2: general path tiles */
		textures_path("tiles/maptile_121.png"),
		textures_path("tiles/maptile_122.png"),
		textures_path("tiles/maptile_123.png"),
		textures_path("tiles/maptile_124.png"),
		textures_path("tiles/maptile_125.png"),
		textures_path("tiles/maptile_126.png"),
		textures_path("tiles/maptile_127.png"),
		textures_path("tiles/maptile_128.png"),
		textures_path("tiles/maptile_129.png"),
		textures_path("tiles/maptile_130.png"),
		textures_path("tiles/maptile_142.png"),
		textures_path("tiles/maptile_143.png"),
		textures_path("tiles/maptile_144.png"),
		textures_path("tiles/maptile_146.png"),
		textures_path("tiles/maptile_147.png"),

		/* A2: start tiles */
		textures_path("tiles/maptile_155.png"),
		textures_path("tiles/maptile_156.png"),
		textures_path("tiles/maptile_159.png"),
		textures_path("tiles/maptile_160.png"),
		textures_path("tiles/maptile_161.png"),
		textures_path("tiles/maptile_162.png"),
		textures_path("tiles/maptile_163.png"),
		textures_path("tiles/maptile_164.png"),
		textures_path("tiles/maptile_172.png"),
		textures_path("tiles/maptile_173.png"),
		textures_path("tiles/maptile_176.png"),
		textures_path("tiles/maptile_177.png"),
		textures_path("tiles/maptile_178.png"),
		textures_path("tiles/maptile_180.png"),
		textures_path("tiles/maptile_181.png"),

		/* A2: exit tiles */
		textures_path("tiles/maptile_255.png"),
		textures_path("tiles/maptile_256.png"),
		textures_path("tiles/maptile_259.png"),
		textures_path("tiles/maptile_260.png"),
		textures_path("tiles/maptile_261.png"),
		textures_path("tiles/maptile_262.png"),
		textures_path("tiles/maptile_263.png"),
		textures_path("tiles/maptile_264.png"),
		textures_path("tiles/maptile_272.png"),
		textures_path("tiles/maptile_273.png"),
		textures_path("tiles/maptile_276.png"),
		textures_path("tiles/maptile_277.png"),
		textures_path("tiles/maptile_278.png"),
		textures_path("tiles/maptile_280.png"),
		textures_path("tiles/maptile_281.png"),
		
		textures_path("projectiles/white_bubble.png")
	};

	std::array<GLuint, effect_count> effects;
	// Make sure these paths remain in sync with the associated enumerators.
	const std::array<std::string, effect_count> effect_paths = {
		shader_path("coloured"),
		shader_path("egg"),
		shader_path("chicken"),
		shader_path("textured"),
		shader_path("vignette")
	};

	std::array<GLuint, geometry_count> vertex_buffers;
	std::array<GLuint, geometry_count> index_buffers;
	std::array<Mesh, geometry_count> meshes;

public:
	// Initialize the window
	bool init(GLFWwindow* window);

	template <class T>
	void bindVBOandIBO(GEOMETRY_BUFFER_ID gid, std::vector<T> vertices, std::vector<uint16_t> indices);

	void initializeGlTextures();

	void initializeGlEffects();

	void initializeGlMeshes();

	Mesh& getMesh(GEOMETRY_BUFFER_ID id) { return meshes[(int)id]; };

	void initializeGlGeometryBuffers();

	// Initialize the screen texture used as intermediate render target
	// The draw loop first renders to this texture, then it is used for the vignette shader
	bool initScreenTexture();

	// Destroy resources associated to one or all entities created by the system
	~RenderSystem();

	// Draw all entities - (A2) based on the game_screen
	void draw(GAME_SCREEN_ID game_screen, HUD hud);

	mat3 createProjectionMatrix();

	Entity get_screen_state_entity() { return screen_state_entity; }

	// A3
	FontSystem font_system;

private:
	// Internal drawing functions for each entity type
	void drawGridLine(Entity entity, const mat3& projection);
	void drawTexturedMesh(Entity entity, const mat3& projection);
	void drawToScreen();
	void drawText(const std::string& text, float x, float y, float scale, const glm::vec3& color);
	void drawHealthBar(vec2 position, vec2 size, const glm::vec3& bar_color, const mat3& projection);

	// Window handle
	GLFWwindow* window;

	// Global VAO required by OpenGL core profile.
	// Saved as a member so draw() can restore it after renderText()
	// unbinds it with glBindVertexArray(0).
	GLuint vao;
	GLuint m_font_VAO = 0;
	GLuint m_font_VBO = 0;

	// Screen texture handles
	GLuint frame_buffer;
	GLuint off_screen_render_buffer_color;
	GLuint off_screen_render_buffer_depth;

	Entity screen_state_entity;
};

bool loadEffectFromFile(
	const std::string& vs_path, const std::string& fs_path, GLuint& out_program);
