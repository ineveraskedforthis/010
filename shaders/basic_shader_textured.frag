#version 330 core
#define PI 3.1415926538

uniform sampler2D texture_sampler;

in vec2 tex_coord;

layout (location = 0) out vec4 out_color;

void main()
{
	out_color = texture(texture_sampler, tex_coord);
	// out_color = vec4(1.f, 0.f, 0.f, 1.f);
}