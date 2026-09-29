// reference: https://canvas.ubc.ca/courses/176823/files/folder/simpleGL?preview=44955771

#include "font_system.hpp"

bool FontSystem::fontInit(GLFWwindow* window, const std::string& font_filename, unsigned int font_default_size, GLuint& font_VAO, GLuint& font_VBO)
{
	(void)window; // window size comes from WINDOW_WIDTH/HEIGHT_PX constants

    // load and compile font shaders via existing loadEffectFromFile
	if (!loadEffectFromFile(
		shader_path("font") + ".vs.glsl",
		shader_path("font") + ".fs.glsl",
		m_font_shaderProgram)) {
		std::cerr << "ERROR: FontSystem: failed to load font shaders" << std::endl;
		return false;
	}


	// A3 TODO: enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl_has_errors(__FILE__, __LINE__);

	// font buffer set up: VAO + VBO
	glGenVertexArrays(1, &font_VAO);
	glGenBuffers(1, &font_VBO);
    gl_has_errors(__FILE__, __LINE__);

	// apply orthographic projection (screen-space, origin = bottom-left)
	glUseProgram(m_font_shaderProgram);
	glm::mat4 projection = glm::ortho(
		0.0f, static_cast<float>(WINDOW_WIDTH_PX),
		0.0f, static_cast<float>(WINDOW_HEIGHT_PX)
	);
	GLint project_location = glGetUniformLocation(m_font_shaderProgram, "projection");
	assert(project_location > -1);
	glUniformMatrix4fv(project_location, 1, GL_FALSE, glm::value_ptr(projection));
    gl_has_errors(__FILE__, __LINE__);

	// init FreeType 
	FT_Library ft;
	if (FT_Init_FreeType(&ft)) {
		std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
		return false;
	}

	FT_Face face;
	if (FT_New_Face(ft, font_filename.c_str(), 0, &face)) {
		std::cerr << "ERROR::FREETYPE: Failed to load font: " << font_filename << std::endl;
		FT_Done_FreeType(ft);
		return false;
	}

	// set glyph pixel height (width=0 means auto)
	FT_Set_Pixel_Sizes(face, 0, font_default_size);

	// disable byte-alignment restriction (FreeType bitmaps are 1-byte aligned)
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gl_has_errors(__FILE__, __LINE__);

	// rasterize and upload all 128 ASCII glyphs
	for (unsigned char c = 0; c < 128; c++)
	{
		if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
			std::cerr << "ERROR::FREETYPE: Failed to load glyph '" << c << "'" << std::endl;
			continue;
		}

		unsigned int texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RED,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0, GL_RED, GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);
        gl_has_errors(__FILE__, __LINE__);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl_has_errors(__FILE__, __LINE__);

		// store character — note uppercase member names matching Character struct
		Character character = {
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left,  face->glyph->bitmap_top),
			static_cast<unsigned int>(face->glyph->advance.x),
			(char)c
		};
		m_ftCharacters.insert(std::pair<char, Character>(c, character));
	}
	glBindTexture(GL_TEXTURE_2D, 0);
    gl_has_errors(__FILE__, __LINE__);

	// clean up FreeType: glyph data is now on the GPU
	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	// set up VAO/VBO for dynamic per-character quads
	// 6 vertices * 4 floats (xy position + zw texcoord), updated per character
	glBindVertexArray(font_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, font_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
    gl_has_errors(__FILE__, __LINE__);

	std::cout << "INFO: FontSystem initialized with: " << font_filename << std::endl;
	return true;
}

void FontSystem::renderText(const std::string& text, float x, float y, float scale, const glm::vec3& color)
{
	// activate font shader and pass text color
	glUseProgram(m_font_shaderProgram);
	glUniform3f(glGetUniformLocation(m_font_shaderProgram, "textColor"), color.r, color.g, color.b);
    gl_has_errors(__FILE__, __LINE__);

	// ensure blending is on (game renderer may have changed state)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl_has_errors(__FILE__, __LINE__);

	glActiveTexture(GL_TEXTURE0);
    gl_has_errors(__FILE__, __LINE__);

	for (const char& c : text)
	{
		Character ch = m_ftCharacters[c];

		float xpos = x + ch.Bearing.x * scale;
		float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

		float w = ch.Size.x * scale;
		float h = ch.Size.y * scale;

		// 2-triangle quad for this glyph (vec4: xy position + uv texcoord)
		float vertices[6][4] = {
			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos,     ypos,       0.0f, 1.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },

			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },
			{ xpos + w, ypos + h,   1.0f, 0.0f }
		};

		// bind this glyph's texture
		glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        gl_has_errors(__FILE__, __LINE__);

		// upload quad vertices into the dynamic VBO
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        gl_has_errors(__FILE__, __LINE__);

		// draw the glyph quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
        gl_has_errors(__FILE__, __LINE__);

		// advance cursor: FreeType advance is in 1/64px, >> 6 converts to pixels
		x += (ch.Advance >> 6) * scale;
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
    gl_has_errors(__FILE__, __LINE__);
}

FontSystem::~FontSystem() {
    // clean up
	for (auto& pair : m_ftCharacters) {
		glDeleteTextures(1, &pair.second.TextureID);
	}
	glDeleteProgram(m_font_shaderProgram);
    gl_has_errors(__FILE__, __LINE__);
}