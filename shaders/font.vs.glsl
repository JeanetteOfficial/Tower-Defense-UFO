#version 330 core

layout(location = 0) in vec4 vertex; // vec4.xyzw = packed vec2 pos (xy) + vec2 tex_coord (zw)
out vec2 TexCoords;

uniform mat4 projection; // convert screen-space pixels to NDC
// uniform mat4 transform; // no need (renderText positions each glyph quad directly in screen space)

// reference: https://learnopengl.com/In-Practice/Text-Rendering
void main()
{
	// A3 TODO
	gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
