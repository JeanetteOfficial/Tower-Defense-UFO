
#include <SDL.h>
#include <glm/trigonometric.hpp>
#include <glm/glm.hpp>
#include <iostream>


// internal
#include "render_system.hpp"
#include "tinyECS/registry.hpp"
#include "world_system.hpp"

void RenderSystem::drawGridLine(Entity entity, const mat3& projection) {

	GridLine& gridLine = registry.gridLines.get(entity);

	// Transformation code, see Rendering and Transformation in the template
	// specification for more info Incrementally updates transformation matrix,
	// thus ORDER IS IMPORTANT
	Transform transform;
	transform.translate(gridLine.start_pos);
	transform.scale(gridLine.end_pos);

	assert(registry.renderRequests.has(entity));
	const RenderRequest& render_request = registry.renderRequests.get(entity);

	const GLuint used_effect_enum = (GLuint)render_request.used_effect;
	assert(used_effect_enum != (GLuint)EFFECT_ASSET_ID::EFFECT_COUNT);
	const GLuint program = (GLuint)effects[used_effect_enum];

	// setting shaders
	glUseProgram(program);
	gl_has_errors(__FILE__, __LINE__);

	assert(render_request.used_geometry != GEOMETRY_BUFFER_ID::GEOMETRY_COUNT);
	const GLuint vbo = vertex_buffers[(GLuint)render_request.used_geometry];
	const GLuint ibo = index_buffers[(GLuint)render_request.used_geometry];

	// Setting vertex and index buffers
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	gl_has_errors(__FILE__, __LINE__);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	gl_has_errors(__FILE__, __LINE__);

	if (render_request.used_effect == EFFECT_ASSET_ID::EGG)
	{
		GLint in_position_loc = glGetAttribLocation(program, "in_position");
		gl_has_errors(__FILE__, __LINE__);

		GLint in_color_loc    = glGetAttribLocation(program, "in_color");
		gl_has_errors(__FILE__, __LINE__);

		glEnableVertexAttribArray(in_position_loc);
		gl_has_errors(__FILE__, __LINE__);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), (void*)0);
		gl_has_errors(__FILE__, __LINE__);

		glEnableVertexAttribArray(in_color_loc);
		glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), (void*)sizeof(vec3));
		gl_has_errors(__FILE__, __LINE__);
	}
	else
	{
		assert(false && "Type of render request not supported");
	}

	// Getting uniform locations for glUniform* calls
	GLint color_uloc = glGetUniformLocation(program, "fcolor");
	const vec3 color = registry.colors.has(entity) ? registry.colors.get(entity) : vec3(1);
	// CK: std::cout << "line color: " << color.r << ", " << color.g << ", " << color.b << std::endl;
	glUniform3fv(color_uloc, 1, (float*)&color);
	gl_has_errors(__FILE__, __LINE__);

	// Get number of indices from index buffer, which has elements uint16_t
	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors(__FILE__, __LINE__);

	GLsizei num_indices = size / sizeof(uint16_t);

	GLint currProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currProgram);
	// Setting uniform values to the currently bound program
	GLuint transform_loc = glGetUniformLocation(currProgram, "transform");
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
	gl_has_errors(__FILE__, __LINE__);

	GLuint projection_loc = glGetUniformLocation(currProgram, "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);
	gl_has_errors(__FILE__, __LINE__);

	// Drawing of num_indices/3 triangles specified in the index buffer
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
	gl_has_errors(__FILE__, __LINE__);
}

void RenderSystem::drawTexturedMesh(Entity entity, const mat3 &projection)
{
	Motion &motion = registry.motions.get(entity);
	// Transformation code, see Rendering and Transformation in the template
	// specification for more info Incrementally updates transformation matrix,
	// thus ORDER IS IMPORTANT
	Transform transform;
	transform.translate(motion.position);
	transform.scale(motion.scale);
	transform.rotate(radians(motion.angle));

	assert(registry.renderRequests.has(entity));
	const RenderRequest &render_request = registry.renderRequests.get(entity);

	const GLuint used_effect_enum = (GLuint)render_request.used_effect;
	assert(used_effect_enum != (GLuint)EFFECT_ASSET_ID::EFFECT_COUNT);
	const GLuint program = (GLuint)effects[used_effect_enum];

	// Setting shaders
	glUseProgram(program);
	gl_has_errors(__FILE__, __LINE__);

	assert(render_request.used_geometry != GEOMETRY_BUFFER_ID::GEOMETRY_COUNT);
	const GLuint vbo = vertex_buffers[(GLuint)render_request.used_geometry];
	const GLuint ibo = index_buffers[(GLuint)render_request.used_geometry];

	// Setting vertex and index buffers
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	gl_has_errors(__FILE__, __LINE__);

	// texture-mapped entities - use data location as in the vertex buffer
	if (render_request.used_effect == EFFECT_ASSET_ID::TEXTURED)
	{
		GLint in_position_loc = glGetAttribLocation(program, "in_position");
		GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
		gl_has_errors(__FILE__, __LINE__);
		assert(in_texcoord_loc >= 0);

		glEnableVertexAttribArray(in_position_loc);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
							  sizeof(TexturedVertex), (void *)0);
		gl_has_errors(__FILE__, __LINE__);

		glEnableVertexAttribArray(in_texcoord_loc);
		glVertexAttribPointer(
			in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex),
			(void *)sizeof(
				vec3)); // note the stride to skip the preceeding vertex position

		// Enabling and binding texture to slot 0
		glActiveTexture(GL_TEXTURE0);
		gl_has_errors(__FILE__, __LINE__);

		assert(registry.renderRequests.has(entity));
		GLuint texture_id =
			texture_gl_handles[(GLuint)registry.renderRequests.get(entity).used_texture];

		glBindTexture(GL_TEXTURE_2D, texture_id);
		gl_has_errors(__FILE__, __LINE__);
	}
	// .obj entities
	else if (render_request.used_effect == EFFECT_ASSET_ID::CHICKEN || render_request.used_effect == EFFECT_ASSET_ID::EGG)
	{
		GLint in_position_loc = glGetAttribLocation(program, "in_position");
		GLint in_color_loc = glGetAttribLocation(program, "in_color");
		gl_has_errors(__FILE__, __LINE__);

		glEnableVertexAttribArray(in_position_loc);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
							  sizeof(ColoredVertex), (void *)0);
		gl_has_errors(__FILE__, __LINE__);

		glEnableVertexAttribArray(in_color_loc);
		glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE,
							  sizeof(ColoredVertex), (void *)sizeof(vec3));
		gl_has_errors(__FILE__, __LINE__);
	}
	else
	{
		assert(false && "Type of render request not supported");
	}

	// Getting uniform locations for glUniform* calls
	GLint color_uloc = glGetUniformLocation(program, "fcolor");
	const vec3 color = registry.colors.has(entity) ? registry.colors.get(entity) : vec3(1);
	glUniform3fv(color_uloc, 1, (float *)&color);
	gl_has_errors(__FILE__, __LINE__);

	// Get number of indices from index buffer, which has elements uint16_t
	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors(__FILE__, __LINE__);

	GLsizei num_indices = size / sizeof(uint16_t);
	// GLsizei num_triangles = num_indices / 3;

	GLint currProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currProgram);
	// Setting uniform values to the currently bound program
	GLuint transform_loc = glGetUniformLocation(currProgram, "transform");
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float *)&transform.mat);
	gl_has_errors(__FILE__, __LINE__);

	GLuint projection_loc = glGetUniformLocation(currProgram, "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float *)&projection);
	gl_has_errors(__FILE__, __LINE__);

	// Drawing of num_indices/3 triangles specified in the index buffer
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
	gl_has_errors(__FILE__, __LINE__);
}

// first draw to an intermediate texture,
// apply the "vignette" texture, when requested
// then draw the intermediate texture
void RenderSystem::drawToScreen()
{
	// Setting shaders
	// get the vignette texture, sprite mesh, and program
	glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::VIGNETTE]);
	gl_has_errors(__FILE__, __LINE__);

	// Clearing backbuffer
	int w, h;
	glfwGetFramebufferSize(window, &w, &h); // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, w, h);
	glDepthRange(0, 10);
	glClearColor(1.f, 0, 0, 1.0);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gl_has_errors(__FILE__, __LINE__);
	// Enabling alpha channel for textures
	glDisable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	// Draw the screen texture on the quad geometry
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[(GLuint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]);
	glBindBuffer(
		GL_ELEMENT_ARRAY_BUFFER,
		index_buffers[(GLuint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]); // Note, GL_ELEMENT_ARRAY_BUFFER associates
																	 // indices to the bound GL_ARRAY_BUFFER
	gl_has_errors(__FILE__, __LINE__);

	// add the "vignette" effect
	const GLuint vignette_program = effects[(GLuint)EFFECT_ASSET_ID::VIGNETTE];

	// set clock
	GLuint time_uloc       = glGetUniformLocation(vignette_program, "time");
	GLuint dead_timer_uloc = glGetUniformLocation(vignette_program, "darken_screen_factor");

	glUniform1f(time_uloc, (float)(glfwGetTime() * 10.0f));
	
	ScreenState &screen = registry.screenStates.get(screen_state_entity);
	// std::cout << "screen.darken_screen_factor: " << screen.darken_screen_factor << " entity id: " << screen_state_entity << std::endl;
	glUniform1f(dead_timer_uloc, screen.darken_screen_factor);
	gl_has_errors(__FILE__, __LINE__);

	// Set the vertex position and vertex texture coordinates (both stored in the
	// same VBO)
	GLint in_position_loc = glGetAttribLocation(vignette_program, "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
	gl_has_errors(__FILE__, __LINE__);

	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);

	glBindTexture(GL_TEXTURE_2D, off_screen_render_buffer_color);
	gl_has_errors(__FILE__, __LINE__);

	// Draw
	glDrawElements(
		GL_TRIANGLES, 3, GL_UNSIGNED_SHORT,
		nullptr); // one triangle = 3 vertices; nullptr indicates that there is
				  // no offset from the bound index buffer
	gl_has_errors(__FILE__, __LINE__);
}

void RenderSystem::drawText(const std::string& text, float x, float y, float scale, const glm::vec3& color)
{
	glBindVertexArray(0);
	glBindVertexArray(m_font_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_font_VBO);

	// A3: Hello World testing
	font_system.renderText(text, x, y, scale, color);

	// restore the global VAO for the next frame's draw calls
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, 0);


}

// using similar logic as drawGridLine
void RenderSystem:: drawHealthBar(vec2 position, vec2 size, const glm::vec3& bar_color, const mat3& projection) {

	// Transformation code, see Rendering and Transformation in the template
	// specification for more info Incrementally updates transformation matrix,
	// thus ORDER IS IMPORTANT
	Transform transform;
	transform.translate(position);
	transform.scale(size);

	// hard coded
	const GLuint program = (GLuint)effects[(GLuint)EFFECT_ASSET_ID::EGG];
	const GLuint vbo = vertex_buffers[(GLuint)GEOMETRY_BUFFER_ID::DEBUG_LINE];
	const GLuint ibo = index_buffers[(GLuint)GEOMETRY_BUFFER_ID::DEBUG_LINE];

	// setting shaders
	glUseProgram(program);
	gl_has_errors(__FILE__, __LINE__);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	gl_has_errors(__FILE__, __LINE__);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	gl_has_errors(__FILE__, __LINE__);

	GLint in_position_loc = glGetAttribLocation(program, "in_position");
	gl_has_errors(__FILE__, __LINE__);

	GLint in_color_loc = glGetAttribLocation(program, "in_color");
	gl_has_errors(__FILE__, __LINE__);

	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), (void*)0);
	gl_has_errors(__FILE__, __LINE__);

	glEnableVertexAttribArray(in_color_loc);
	glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), (void*)sizeof(vec3));
	gl_has_errors(__FILE__, __LINE__);

	// Getting uniform locations for glUniform* calls
	GLint color_uloc = glGetUniformLocation(program, "fcolor");
	// const vec3 color = registry.colors.has(entity) ? registry.colors.get(entity) : vec3(1);
	// CK: std::cout << "line color: " << color.r << ", " << color.g << ", " << color.b << std::endl;
	glUniform3fv(color_uloc, 1, (float*)&bar_color);
	gl_has_errors(__FILE__, __LINE__);

	// Get number of indices from index buffer, which has elements uint16_t
	GLint buffer_size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &buffer_size);
	gl_has_errors(__FILE__, __LINE__);

	GLsizei num_indices = buffer_size / sizeof(uint16_t);

	GLint currProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currProgram);
	// Setting uniform values to the currently bound program
	GLuint transform_loc = glGetUniformLocation(currProgram, "transform");
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform.mat);
	gl_has_errors(__FILE__, __LINE__);

	GLuint projection_loc = glGetUniformLocation(currProgram, "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);
	gl_has_errors(__FILE__, __LINE__);

	// Drawing of num_indices/3 triangles specified in the index buffer
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
	gl_has_errors(__FILE__, __LINE__);

}

// Render our game world
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/
void RenderSystem::draw(GAME_SCREEN_ID game_screen, HUD hud)
{

	// save the level name for text rendering later
	std::string game_lv = hud.level_filename.substr(0, 6);

	// Getting size of window
	int w, h;
	glfwGetFramebufferSize(window, &w, &h); // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays

	// First render to the custom framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	glViewport(0, 0, w, h);
	glDepthRange(0.00001, 10);
	gl_has_errors(__FILE__, __LINE__);

	// A2: change background based on game screen
	switch (game_screen) {
		case GAME_SCREEN_ID::INTRO:
			// A3: dark blue background
			glClearColor(0.05f, 0.05f, 0.2f, 1.0f); 
    		break;
		case GAME_SCREEN_ID::DRAWING:
			// white background
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			break;

		case GAME_SCREEN_ID::PLAYING:
			// light green background
			glClearColor((147.0f / 255.0f), (255.0f / 255.0f), (174.0f / 255.0f), 1.0f);
			break;

		case GAME_SCREEN_ID::TILE_SELECTOR:
			// light yellow background
			glClearColor((242.0f / 255.0f), (255.0f / 255.0f), (104.0f / 255.0f), 1.0f);
			break;
		case GAME_SCREEN_ID::GAME_OVER:
			// A3: black background
			glClearColor(0.f, 0.0f, 0.0f, 1.0f);
    		break;
		default:
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			break;
	}

	// clear backbuffer
	glClearDepth(10.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST); // native OpenGL does not work with a depth buffer
							  // and alpha blending, one would have to sort
							  // sprites back to front
	gl_has_errors(__FILE__, __LINE__);
	

	mat3 projection_2D = createProjectionMatrix();
	// A2: draw gridlines first
	for (auto entity : registry.gridLines.entities) {
		if (!registry.filledTiles.has(entity)) // skip filledTiles
			drawGridLine(entity, projection_2D);
	}
	// A2: draw filledTiles next
	// draw any filledTiles here so they appear behind the main, but above the grid lines
	for (auto entity : registry.filledTiles.entities) {
		drawGridLine(entity, projection_2D);
	}
	// A2: draw everything else over top of the previous items
	// draw all entities with a render request to the frame buffer
	// Note, its not very efficient to access elements indirectly via the entity
	// albeit iterating through all Sprites in sequence. A good point to optimize
	for (Entity entity : registry.renderRequests.entities)
	{
		// filter to entities that have a motion component (legacy), but are not selectable
		// A2: draw map for drawing or playing (expect selectables)
		if ((game_screen == GAME_SCREEN_ID::DRAWING
			|| game_screen == GAME_SCREEN_ID::PLAYING)
			&& registry.motions.has(entity)
			&& !registry.selectables.has(entity)) {
			drawTexturedMesh(entity, projection_2D);
		}

		// A2: draw the selectable tiles on the tile-selector screen
		if ((game_screen == GAME_SCREEN_ID::TILE_SELECTOR)
			&& registry.motions.has(entity)
			&& registry.selectables.has(entity)) {
			drawTexturedMesh(entity, projection_2D);
		}
		
	}

	// draw framebuffer to screen
	// adding "vignette" effect when applied
	drawToScreen();

	// testing text render
	// drawText("Hello World", 50.f, 50.f, 1.0f, glm::vec3(1.0f, 0.0f, 0.0f));

	// A3 TODO: screen-dependent text rendering (origin is bottom-left, y increases upward)
	switch (game_screen) {
		case GAME_SCREEN_ID::INTRO:
			drawText("TOWERS VS ALIENS", 220.f, 480.f, 2.0f,  glm::vec3(1.0f, 1.0f, 0.2f));
			drawText("Student name: Jeanette Hu", 220.f, 430.f, 0.8f,  glm::vec3(1.0f, 1.0f, 0.2f));
			// TODO A3
			drawText("High Score: " + std::to_string(hud.high_score), 220.f, 380.f, 0.8f,  glm::vec3(1.0f, 1.0f, 0.2f));
			drawText("Game level: <" + game_lv + "> (select level by pressing on number key 0 ~ 9)", 220.f, 330.f, 0.8f,  glm::vec3(1.0f, 1.0f, 0.2f));
			drawText("Space - start the game", 220.f, 280.f, 0.7f, glm::vec3(1.0f, 1.0f, 1.0f));
			drawText("D - load the current level in draw mode", 220.f, 220.f, 0.7f, glm::vec3(1.0f, 1.0f, 1.0f));
			drawText("E - draw mode to switch to and from the tile-selector screen", 220.f, 190.f, 0.7f, glm::vec3(1.0f, 1.0f, 1.0f));
			drawText("G - generate a map and start", 220.f, 250.f, 0.7f, glm::vec3(1.0f, 1.0f, 1.0f));
			drawText("P - start the game", 220.f, 160.f, 0.7f, glm::vec3(1.0f, 1.0f, 1.0f));
			drawText("R - reset the current level", 220.f, 130.f, 0.7f, glm::vec3(1.0f, 1.0f, 1.0f));
			drawText("ESC - quit OR in all other screens to switch back to intro screen", 220.f, 100.f, 0.7f, glm::vec3(1.0f, 1.0f, 1.0f));
    		break;

		case GAME_SCREEN_ID::DRAWING:
			break;

		case GAME_SCREEN_ID::PLAYING:
			// HUD:
			// show error message on top row if no valid path is found
			if (!hud.valid_path) {
				drawText("No valid path is found, therefore cannot start playing!", 10.f, 670.f, 0.7f, glm::vec3(1.0f, 0.0f, 0.0f));
			}
			else {
				// TODO A3: show current points on top row
				drawText("Score: " + std::to_string(hud.curr_score) + "; ", 10.f, 670.f, 0.8f, glm::vec3(1.0f, 1.0f, 1.0f));
				drawText("HIGH SCORE: " + std::to_string(hud.high_score) + "; ", 150.f, 670.f, 0.8f, glm::vec3(1.0f, 1.0f, 1.0f));
				drawText("Unspawned: " + std::to_string(hud.unspawned) + "; ", 410.f, 670.f, 0.8f, glm::vec3(1.0f, 1.0f, 1.0f));
				drawText("Max Towers: " + std::to_string(hud.max_towers) + "; ", 700.f, 670.f, 0.8f, glm::vec3(1.0f, 1.0f, 1.0f));
				drawText("Tower Countdown: " + std::to_string(hud.tower_countdown_ms) + "(ms); ", 890.f, 670.f, 0.8f, glm::vec3(1.0f, 1.0f, 1.0f));

				// TODO A3: show the invader's points and health bar above their head
				for (Entity &entity : registry.invaders.entities) {
					// get the Motion component
					Motion& motion = registry.motions.get(entity);
					// get the Invader component to have its random_point
					Invader& invader = registry.invaders.get(entity);
					int invader_point = invader.random_point;
					float health_pct = invader.health / (float)INVADER_HEALTH; 
					// calculate text_x and text_y
					float text_x = motion.position.x - 26.f; // top left side
					float text_y = WINDOW_HEIGHT_PX - motion.position.y + (INVADER_BB_HEIGHT / 2.f); // convert invader pos to text pos
					// call drawText
					drawText(std::to_string(invader_point), text_x, text_y, 0.5f, glm::vec3(0.0f, 0.0f, 0.0f));
					// determine health bar color and width based on health_pct
					glm::vec3 bar_color = glm::vec3(0.0f, 1.0f, 0.0f); // green by default
					if (health_pct >= 0.75f && health_pct <= 1.f) {
						bar_color = glm::vec3(0.0f, 1.0f, 0.0f); // green
					} 
					else if (health_pct >= 0.25f && health_pct < 0.75f) {
						bar_color = glm::vec3(1.0f, 1.0f, 0.0f); // yellow
					} 
					else { // lower than 25%
						bar_color = glm::vec3(1.0f, 0.0f, 0.0f); // red
					}
					// bar width & height -> size
					float bar_width = INVADER_BB_WIDTH * health_pct;
					float bar_height = 8.f;
					// bar pos
					float bar_x = text_x + 40.f - (INVADER_BB_WIDTH - bar_width) / 2.f; // to the right of the point's text
					float bar_y = motion.position.y - (INVADER_BB_HEIGHT / 2.f); 
					// call drawHealthBar
					drawHealthBar(vec2(bar_x, bar_y), vec2(bar_width, bar_height), bar_color, projection_2D);
				}
				// invader points floating effect
				for (const FloatingText &ft: hud.floating_texts) {
					if (ft.active) {
						drawText(ft.point, ft.x, ft.y, 0.5f, glm::vec3(0.0f, 0.0f, 0.0f));
					}
				}
				// show the new high score alert
				if (hud.show_new_high_score_alert) {
					// flash between yellow and white every 250ms
					bool flash = fmod(hud.new_high_score_timer_ms, 500.f) > 250.f;
					glm::vec3 flash_color = flash ? glm::vec3(1.f, 1.f, 0.f) : glm::vec3(1.f, 1.f, 1.f);
					drawText("NEW HIGH SCORE!!!", 460.f, 360.f, 2.f, flash_color);
				}
				// TODO A3: show tower's health bar above their head
				for (Entity &entity : registry.towers.entities) {
					// get the Motion component
					Motion& motion = registry.motions.get(entity);
					Tower& tower = registry.towers.get(entity);
					float health_pct = tower.health / (float)TOWER_HEALTH; 
					// calculate text_x and text_y
					float text_x = motion.position.x - 26.f; // top left side
					float text_y = WINDOW_HEIGHT_PX - motion.position.y + (INVADER_BB_HEIGHT / 2.f); // convert tower pos to text pos
					// determine health bar color and width based on health_pct
					glm::vec3 bar_color = glm::vec3(0.0f, 1.0f, 0.0f); // green by default
					if (health_pct >= 0.75f && health_pct <= 1.f) {
						bar_color = glm::vec3(0.0f, 1.0f, 0.0f); // green
					} 
					else if (health_pct >= 0.25f && health_pct < 0.75f) {
						bar_color = glm::vec3(1.0f, 1.0f, 0.0f); // yellow
					} 
					else { // lower than 25%
						bar_color = glm::vec3(1.0f, 0.0f, 0.0f); // red
					}
					// bar width & height -> size
					float bar_width = TOWER_BB_WIDTH * health_pct;
					float bar_height = 8.f;
					// bar pos
					float bar_x = text_x + 22.f - (TOWER_BB_WIDTH - bar_width) / 2.f; 
					float bar_y = motion.position.y - (TOWER_BB_HEIGHT / 2.f); 
					// call drawHealthBar
					drawHealthBar(vec2(bar_x, bar_y), vec2(bar_width, bar_height), bar_color, projection_2D);
				}
			}
			break;

		case GAME_SCREEN_ID::TILE_SELECTOR:
			break;

		case GAME_SCREEN_ID::GAME_OVER:
			// A3 TODO: either defeat or victory
			if (!hud.is_victory) {
				// DEFEAT
				drawText("GAME OVER", 420.f, 450.f, 1.2f,  glm::vec3(1.0f, 0.f, 0.f)); // in red
				drawText("DEFEAT!", 420.f, 400.f, 1.5f,  glm::vec3(1.f, 0.f, 0.f)); // in red
			}
			else {
				// VICTORY
				drawText("GAME OVER", 420.f, 450.f, 1.2f,  glm::vec3(0.f, 1.0f, 0.f)); // in green
				drawText("VICTORY!!!", 420.f, 400.f, 1.2f,  glm::vec3(0.f, 1.0f, 0.f)); // in green

			}
			// display current high score no matter defeat or victory
			drawText("Current High Score: " + std::to_string(hud.high_score), 420.f, 350.f, 0.9f, glm::vec3(1.0f, 1.0f, 1.0f));
			// display new high score if the player reached one
			if (hud.new_high_score) {
				drawText("NEW HIGH SCORE!!!", 460.f, 320.f, 0.8f, glm::vec3(1.0f, 1.0f, 0.f));
			}
			else {
				// player's score
				drawText("Your Score: " + std::to_string(hud.curr_score), 420.f, 320.f, 0.8f, glm::vec3(1.0f, 1.0f, 1.0f));
			}
			drawText("ESC - go back to intro page", 420.f, 270.f, 0.7f, glm::vec3(1.0f, 1.0f, 1.0f));
    		break;
	}

	// flicker-free display with a double buffer
	glfwSwapBuffers(window);
	gl_has_errors(__FILE__, __LINE__);
}

mat3 RenderSystem::createProjectionMatrix()
{
	// fake projection matrix, scaled to window coordinates
	float left   = 0.f;
	float top    = 0.f;
	float right  = (float) WINDOW_WIDTH_PX;
	float bottom = (float) WINDOW_HEIGHT_PX;

	float sx = 2.f / (right - left);
	float sy = 2.f / (top - bottom);
	float tx = -(right + left) / (right - left);
	float ty = -(top + bottom) / (top - bottom);

	return {
		{ sx, 0.f, 0.f},
		{0.f,  sy, 0.f},
		{ tx,  ty, 1.f}
	};
}