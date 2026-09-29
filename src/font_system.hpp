// reference: https://canvas.ubc.ca/courses/176823/files/folder/simpleGL?preview=44955771

#pragma once

#include "common.hpp"

// fonts
#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>				// map of character textures
#include <string>
#include <iostream>
#include <assert.h>
#include <fstream>			// for ifstream
#include <sstream>			// for ostringstream

#include "../ext/project_path.hpp"		// built by CMake, contains project path

// matrices
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


// forward declare (defined in render_system_init.cpp)
bool loadEffectFromFile(const std::string& vs_path, const std::string& fs_path, GLuint& out_program);


// font character structure (all the data needed to render a single glyph)
struct Character {
	unsigned int TextureID;  // ID handle of the glyph texture
	glm::ivec2   Size;       // Size of glyph
	glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
	unsigned int Advance;    // Offset to advance to next glyph
	char character;
};

class FontSystem {

public:

    // Initialize FreeType, load the .ttf at font_filename, compile the font
	// shaders, upload all 128 ASCII glyphs as GL textures, and set up VAO/VBO.
	// Call once from RenderSystem::init().
    bool fontInit(GLFWwindow* window, const std::string& font_filename, unsigned int font_default_size, GLuint& font_VAO, GLuint& font_VBO);
    
    // Render text to the screen in screen-space pixels
    void renderText(const std::string& text, float x, float y, float scale, const glm::vec3& color);

    ~FontSystem();

private:

	std::map<char, Character> m_ftCharacters;
	GLuint m_font_shaderProgram = 0;
	// GLuint m_font_VAO = 0;
	// GLuint m_font_VBO = 0;

};