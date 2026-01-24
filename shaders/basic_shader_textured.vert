#version 330 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 tex_coord_vertex;

out vec2 tex_coord;

void main()
{
	gl_Position = projection * view * model * vec4(in_position, 1.0);
	tex_coord = tex_coord;
}