#version 330 core

in vec2 TexCoords; 
out vec4 color; 

uniform sampler2D text; 
uniform vec3 textColor; 

// reference: https://learnopengl.com/In-Practice/Text-Rendering
void main()
{
    // A3 TODO
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
    color = vec4(textColor, 1.0) * sampled;
    // fully transparent outside the glyph, textColor at full opacity inside, with smooth antialiased edges in between
}
