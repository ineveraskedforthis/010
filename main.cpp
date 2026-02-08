#include "lua.hpp"

#define GLM_FORCE_SWIZZLE
#define GLEW_STATIC

#include <array>
#include <assert.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdexcept>

#include <string>
#include <string_view>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <random>

#include "stb_image/stb_image.h"

// Include all GLM core / GLSL features
#include <glm/glm.hpp> // vec2, vec3, mat4, radians
// Include all GLM extensions
#include <glm/ext.hpp> // perspective, translate, rotate
#include "glm/ext/matrix_transform.hpp"

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "data.hpp"
#include "frustum.hpp"
#include "unordered_dense.h"

#define DCON_LUADLL_EXPORTS
#include "export_ifdefs.hpp"

#include "uitemplate.hpp"
#include "project_description.hpp"

#include "from_alice_editor_main.hpp"
#include "fonts.hpp"
#include "opengl_wrapper.hpp"
#include "text.hpp"
#include "locale.hpp"
#include "gui_graphics.hpp"
#include "alice_ui.hpp"
#include "text_render.hpp"
#include "window.hpp"
#include "ui_containers.hpp"


uint64_t available_window_id = 1;


// https://stackoverflow.com/a/16323388/10281950
static int traceback(lua_State *L) {
	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	lua_getfield(L, -1, "traceback");
	lua_pushvalue(L, 1);
	lua_pushinteger(L, 2);
	lua_call(L, 2, 1);
	auto error = lua_tostring(L, -1);
	window::emit_error_message(error, false);
	fprintf(stderr, "%s\n", error);
	return 1;
}

glm::vec3 hsv_to_rgb(float h, float s, float v) {
	auto c = v * s;
	int section = h / 60;
	auto x = c * (1 - abs(section % 2 - 1));
	auto m = v - c;
	if (h < 60) return {c + m, x + m, 0 + m};
	if (h < 120) return {x + m, c + m, 0 + m};
	if (h < 180) return {0 + m, c + m, x + m};
	if (h < 240) return {0 + m, x + m, c + m};
	if (h < 300) return {x + m, 0 + m, c + m};
	return {c + m, 0 + m, x + m};
}

glm::vec3 rgb_to_hsv(float r, float g, float b) {
	auto max = std::max(r, std::max(g, b));
	auto min = std::min(r, std::min(g, b));

	auto h = 0;
	if (max == min) {
		h = 0;
	} else if (max == r) {
		h = fmod((g - b) * 60 / (max - min), 360.f);
	} else if (max == g) {
		h = fmod((b - r) * 60 / (max - min) + 120, 360.f);
	} else if (max == b) {
		h = fmod((r - g) * 60 / (max - min) + 240, 360.f);
	}
	float s = 0.f;
	if (max != 0) s = 1 - min / max;

	return {h, s, max};
}

int rgb_to_id(int r, int g, int b) {
	return r + 256 * g + 256 * 256 * b;
}

struct settings {
	float ui_scale;
};

namespace game {

constexpr int CHUNK_SIZE = 32;
constexpr int CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;

constexpr int WORLD_RADIUS = 16;
constexpr int WORLD_SIZE = WORLD_RADIUS * 2;
constexpr int WORLD_AREA = WORLD_SIZE * WORLD_SIZE;

constexpr int WORLD_SIZE_TILES = CHUNK_SIZE * WORLD_SIZE;
constexpr int WORLD_AREA_TILES = WORLD_SIZE_TILES * WORLD_SIZE_TILES;

struct vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texcoord;
	int face;
};

struct mesh {
	std::vector<vertex> data;
	GLuint vao;
	GLuint vbo;
};

struct simple_vertex {
	glm::vec3 position;
	glm::vec2 texcoord;
};

struct simple_mesh {
	std::vector<simple_vertex> data;
	GLuint vao;
	GLuint vbo;
};

struct map_state {
	// std::array<char, WORLD_AREA_TILES> height {};
	mesh mesh {};
};

constexpr inline uint32_t TEXT_KEY_IS_TEXTURE_PATH = 1;

struct text_collection {
	std::vector<char> text;
	std::vector<uint32_t> word_start;
	std::vector<uint32_t> word_length;
	std::vector<uint32_t> associated_texture;
	std::vector<uint32_t> associated_texture_width;
	std::vector<uint32_t> associated_texture_height;
	std::vector<uint32_t> flags;
	uint32_t available_key;
	ankerl::unordered_dense::map<std::string, uint32_t> existing_asset;
};

struct state {
	map_state map;
	map_state sky;

	std::default_random_engine rng {};
	std::uniform_real_distribution<float> uniform{0.0, 1.0};
	std::normal_distribution<float> normal {0.f, 1.f};
};

}

enum class race_table_columns_id {
	race_id,
	icon,
	name,
	males_per_hundred_females,
	child_age,
	teen_age,
	middle_age,
	elder_age,
	max_age,
	minimum_comfortable_temperature,
	female_body_size,
	male_body_size
};

std::string_view opengl_get_error_name(GLenum t) {
	switch(t) {
		case GL_INVALID_ENUM:
			return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:
			return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION:
			return "GL_INVALID_OPERATION";
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			return "GL_INVALID_FRAMEBUFFER_OPERATION";
		case GL_OUT_OF_MEMORY:
			return "GL_OUT_OF_MEMORY";
		case GL_STACK_UNDERFLOW:
			return "GL_STACK_UNDERFLOW";
		case GL_STACK_OVERFLOW:
			return "GL_STACK_OVERFLOW";
		case GL_NO_ERROR:
			return "GL_NO_ERROR";
		default:
			return "Unknown";
	}
}
std::string to_string(std::string_view str) {
	return std::string(str.begin(), str.end());
}
void opengl_error_print(std::string message) {
	std::string full_message = message;
	full_message += "\n";
	full_message += opengl_get_error_name(glGetError());
	printf("%s\n", ("OpenGL error:" + full_message).c_str());
}
void assert_no_errors() {
	auto error = glGetError();
	if (error != GL_NO_ERROR) {
		auto message = opengl_get_error_name(error);
		printf("%s\n", (to_string(message)).c_str());
		assert(false);
	}
}

const std::string read_shader(const std::string path) {
	std::string shader_source;
	std::ifstream shader_file;

	shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try {
		shader_file.open(path);
		std::stringstream shader_source_stream;
		shader_source_stream << shader_file.rdbuf();
		shader_file.close();
		shader_source = shader_source_stream.str();
	} catch (std::ifstream::failure& e) {
		throw std::runtime_error(e);
	}

	return shader_source;
}

GLuint create_shader(GLenum type, const char *source) {
	GLuint result = glCreateShader(type);
	glShaderSource(result, 1, &source, nullptr);
	glCompileShader(result);
	GLint status;
	glGetShaderiv(result, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		GLint info_log_length;
		glGetShaderiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
		std::string info_log(info_log_length, '\0');
		glGetShaderInfoLog(result, info_log.size(), nullptr, info_log.data());
		throw std::runtime_error("Shader compilation failed: " + info_log);
	}
	return result;
}

template <typename ... Shaders>
GLuint create_program(Shaders ... shaders)
{
	GLuint result = glCreateProgram();
	(glAttachShader(result, shaders), ...);
	glLinkProgram(result);

	GLint status;
	glGetProgramiv(result, GL_LINK_STATUS, &status);
	if (status != GL_TRUE)
	{
		GLint info_log_length;
		glGetProgramiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
		std::string info_log(info_log_length, '\0');
		glGetProgramInfoLog(result, info_log.size(), nullptr, info_log.data());
		throw std::runtime_error("Program linkage failed: " + info_log);
	}

	return result;
}


void glew_fail(std::string_view message, GLenum error) {
	throw std::runtime_error(to_string(message) + reinterpret_cast<const char *>(glewGetErrorString(error)));
}

void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

static int current_move_x = 0;
static int current_move_y = 0;
static int desired_zoom_level = 0;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	if (key == GLFW_KEY_UP) {
		if (action == GLFW_PRESS) {
			current_move_y += 1;
		} else if (action == GLFW_RELEASE) {
			current_move_y -= 1;
		}
	}

	if (key == GLFW_KEY_DOWN) {
		if (action == GLFW_PRESS) {
			current_move_y -= 1;
		} else if (action == GLFW_RELEASE) {
			current_move_y += 1;
		}
	}

	if (key == GLFW_KEY_LEFT) {
		if (action == GLFW_PRESS) {
			current_move_x -= 1;
		} else if (action == GLFW_RELEASE) {
			current_move_x += 1;
		}
	}

	if (key == GLFW_KEY_RIGHT) {
		if (action == GLFW_PRESS) {
			current_move_x += 1;
		} else if (action == GLFW_RELEASE) {
			current_move_x -= 1;
		}
	}

	if (key == GLFW_KEY_J) {
		if (action == GLFW_PRESS) {
			desired_zoom_level = std::max(-2, desired_zoom_level - 1);
		}
	}

	if (key == GLFW_KEY_K) {
		if (action == GLFW_PRESS) {
			desired_zoom_level  = std::min(1, desired_zoom_level + 1);
		}
	}
}

namespace cubeworld {
	namespace top {
		constexpr glm::vec3 origin = {-1.f, 1.f, -1.f};
		constexpr glm::vec3 center = {0.f, 1.f, 0.f};
		constexpr glm::vec3 ds = {2.f, 0.f, 0.f};
		constexpr glm::vec3 dt = {0.f, 0.f, 2.f};
		constexpr int face = 0;
	}
	namespace bottom {
		constexpr glm::vec3 origin = {1.f, -1.f, -1.f};
		constexpr glm::vec3 center = {0.f, -1.f, 0.f};
		constexpr glm::vec3 ds = {-2.f, 0.f, 0.f};
		constexpr glm::vec3 dt = {0.f, 0.f, 2.f};
		constexpr int face = 1;
	}
	namespace left {
		constexpr glm::vec3 origin = {-1.f, 1.f, 1.f};
		constexpr glm::vec3 center = {-1.f, 0.f, 0.f};
		constexpr glm::vec3 ds = {0.f, -2.f, 0.f};
		constexpr glm::vec3 dt = {0.f, 0.f, -2.f};
		constexpr int face = 2;
	}
	namespace right {
		constexpr glm::vec3 origin = {1.f, -1.f, 1.f};
		constexpr glm::vec3 center = {1.f, 0.f, 0.f};
		constexpr glm::vec3 ds = {0.f, 2.f, 0.f};
		constexpr glm::vec3 dt = {0.f, 0.f, -2.f};
		constexpr int face = 3;
	}
	namespace forward {
		constexpr glm::vec3 origin = {-1.f, -1.f, 1.f};
		constexpr glm::vec3 center = {0.f, 0.f, 1.f};
		constexpr glm::vec3 ds = {0.f, 2.f, 0.f};
		constexpr glm::vec3 dt = {2.f, 0.f, 0.f};
		constexpr int face = 4;
	}
	namespace back {
		constexpr glm::vec3 origin = {1.f, -1.f, -1.f};
		constexpr glm::vec3 center = {0.f, 0.f, -1.f};
		constexpr glm::vec3 ds = {0.f, 2.f, 0.f};
		constexpr glm::vec3 dt = {-2.f, 0.f, 0.f};
		constexpr int face = 5;
	}
}

constexpr glm::vec3 face_to_center[6] {
	cubeworld::top::center,
	cubeworld::bottom::center,
	cubeworld::left::center,
	cubeworld::right::center,
	cubeworld::forward::center,
	cubeworld::back::center,
};
constexpr glm::vec3 face_to_origin[6] {
	cubeworld::top::origin,
	cubeworld::bottom::origin,
	cubeworld::left::origin,
	cubeworld::right::origin,
	cubeworld::forward::origin,
	cubeworld::back::origin,
};
constexpr glm::vec3 face_to_ds[6] {
	cubeworld::top::ds,
	cubeworld::bottom::ds,
	cubeworld::left::ds,
	cubeworld::right::ds,
	cubeworld::forward::ds,
	cubeworld::back::ds,
};
constexpr glm::vec3 face_to_dt[6] {
	cubeworld::top::dt,
	cubeworld::bottom::dt,
	cubeworld::left::dt,
	cubeworld::right::dt,
	cubeworld::forward::dt,
	cubeworld::back::dt,
};

glm::vec3 fst_to_sphere(int world_size, glm::ivec3 fst) {
	int f =fst.x;
	int s = fst.y;
	int t = fst.z;
	auto fs = (float(s) + 0.5f) / float(world_size);
	auto ft = (float(t) + 0.5f) / float(world_size);
	glm::vec3 ds;
	glm::vec3 dt;
	glm::vec3 origin;
	if (f == 0) {
		origin = cubeworld::top::origin;
		dt = cubeworld::top::dt;
		ds = cubeworld::top::ds;
	} else if (f == 1) {
		origin = cubeworld::bottom::origin;
		dt = cubeworld::bottom::dt;
		ds = cubeworld::bottom::ds;
	} else if (f == 2) {
		origin = cubeworld::left::origin;
		dt = cubeworld::left::dt;
		ds = cubeworld::left::ds;
	} else if (f == 3) {
		origin = cubeworld::right::origin;
		dt = cubeworld::right::dt;
		ds = cubeworld::right::ds;
	} else if (f == 4) {
		origin = cubeworld::forward::origin;
		dt = cubeworld::forward::dt;
		ds = cubeworld::forward::ds;
	} else if (f == 5) {
		origin = cubeworld::back::origin;
		dt = cubeworld::back::dt;
		ds = cubeworld::back::ds;
	} else {
		assert(false);
		exit(1);
	}

	auto result = (origin + ds * fs + dt * ft);

	return result / glm::length(result);
}

uint8_t sphere_to_face(glm::vec3 point) {
	// we do it the most stupid way possible: find the closest +- basis vector
	auto best_distance = 3.f;
	auto best_face = 0;
	for (int face = 0; face < 6; ++face) {
		auto distance = glm::distance(point, face_to_center[face]);
		if (distance < best_distance) {
			best_face = face;
			best_distance = distance;
		}
	}
	return best_face;
}

glm::vec3 sphere_to_box(glm::vec3 point) {
	auto face = sphere_to_face(point / glm::length(point));
	auto center = face_to_center[face];
	// move back to cube:
	auto value = glm::dot(center, point);
	auto box_side = point / value;
	return box_side;
}



glm::ivec3 sphere_to_fst(int world_size, glm::vec3 point) {
	auto face = sphere_to_face(point / glm::length(point));
	auto center = face_to_center[face];
	// move back to cube:
	auto value = glm::dot(center, point);
	auto box_side = point / value;
	auto ratio = (box_side - face_to_origin[face]);
	auto dual_dt = face_to_dt[face] / glm::dot(face_to_dt[face], face_to_dt[face]);
	auto dual_ds = face_to_ds[face] / glm::dot(face_to_ds[face], face_to_ds[face]);
	auto t = std::max(0.f, glm::dot(ratio, dual_dt) - 0.00001f) * world_size;
	auto s = std::max(0.f, glm::dot(ratio, dual_ds) - 0.00001f) * world_size;
	return {face, (int)(s), (int)(t)};
}

dcon::tile_id fst_to_tile(int world_size, glm::ivec3 fst ) {
	return dcon::tile_id {(uint32_t) fst.x * world_size * world_size + fst.z * world_size + fst.y};
}

dcon::tile_id r3_to_tile(int world_size, glm::vec3 point) {
	auto fst = sphere_to_fst(world_size, point / glm::length(point));
	return fst_to_tile(world_size, fst);
}

glm::ivec3 tile_to_fst(int world_size, dcon::tile_id tile) {
	auto index = tile.index();
	auto face_size = world_size * world_size;
	auto face = index / face_size;
	auto st = index - face * face_size;
	auto t = st / world_size;
	auto s = st - t * world_size;

	return {face, s, t};
}

glm::vec3 tile_to_sphere(int world_size, dcon::tile_id tile) {
	auto index = tile.index();
	auto face_size = world_size * world_size;
	auto face = index / face_size;
	auto st = index - face * face_size;
	auto t = st / world_size;
	auto s = st - t * world_size;
	return fst_to_sphere(world_size, {face, s, t});
}


glm::vec2 sphere_to_rect(glm::vec3 point) {
	auto d = glm::length(point);
	auto y = acosf(point.y / d);
	auto x = atan2f(point.z, point.x);
	glm::vec2 res = {  x + glm::pi<float>() , y};
	glm::vec2 scaling = { glm::pi<float>() * 2, glm::pi<float>()};
	return res / scaling;
}

int rect_to_image_index(int width, int height, glm::vec2 point) {
	int x = (int)((float)width * point.x);
	int y = (int)((float)height * point.y);
	return width * y + x;
}

float opengl_elevation(float elevation) {
	return (elevation + 32000.f * 2.f) / 32000.f / 2.f;
}

void push_face_vertices(dcon::data_container& state, game::map_state& data, int world_size, glm::vec3 origin, glm::vec3 ds, glm::vec3 dt, uint8_t face, bool fake) {
	auto& mesh = data.mesh.data;

	int N = 256;
	if (fake) {
		N = 64;
	}
	const float Nf = (float) N;
	for (int is = 0; is < N; is++) {
		for (int it = 0; it < N; it++) {

			auto current_s = (float)(is) / Nf;
			auto current_t = (float)(it) / Nf;

			auto next_s = current_s + 1.f / Nf;
			auto next_t = current_t + 1.f / Nf;


			auto s_current_ds = current_s * ds;
			auto s_current_dt = current_t * dt;

			auto s_next_ds = 1.f / Nf * ds;
			auto s_next_dt = 1.f / Nf * dt;

			auto current_origin = origin + s_current_ds + s_current_dt;

			auto minor_shift = glm::vec3(0.00f, 0.00f, 0.00f);

			auto p00 = (current_origin) / glm::length(current_origin);
			auto p01 = (current_origin + s_next_dt) / glm::length(current_origin + s_next_dt);
			auto p10 = (current_origin + s_next_ds) / glm::length(current_origin + s_next_ds);
			auto p11 = (current_origin + s_next_dt + s_next_ds) / glm::length(current_origin + s_next_dt + s_next_ds);

			auto tile00 = r3_to_tile(world_size, p00 + minor_shift);
			auto tile01 = r3_to_tile(world_size, p01 + minor_shift);
			auto tile10 = r3_to_tile(world_size, p10 + minor_shift);
			auto tile11 = r3_to_tile(world_size, p11 + minor_shift);

			auto location = tile_to_sphere(world_size, tile00);
			auto distance = glm::distance(p00, location);
			assert(distance < 4.f / (float)world_size);


			if (fake) {
				p00 *= 1.5f;
				p01 *= 1.5f;
				p10 *= 1.5f;
				p11 *= 1.5f;
			} else {
				auto elevation00 = opengl_elevation(state.tile_get_elevation(tile00));
				auto elevation01 = opengl_elevation(state.tile_get_elevation(tile01));
				auto elevation10 = opengl_elevation(state.tile_get_elevation(tile10));
				auto elevation11 = opengl_elevation(state.tile_get_elevation(tile11));
				p00 *= elevation00;
				p01 *= elevation01;
				p10 *= elevation10;
				p11 *= elevation11;
			}


			auto normal1 = glm::normalize(glm::cross(p01 - p00, p10 - p00));

			mesh.push_back({p00, normal1, {current_s, current_t}, face});
			mesh.push_back({p01, normal1, {current_s, next_t}, face});
			mesh.push_back({p10, normal1, {next_s, current_t}, face});

			auto normal2 = glm::normalize(glm::cross(p01 - p10, p11 - p10));

			mesh.push_back({p10, normal2, {next_s, current_t}, face});
			mesh.push_back({p01, normal2, {current_s, next_t}, face});
			mesh.push_back({p11, normal2, {next_s, next_t}, face});
		}
	}

}

void generate_square(game::simple_mesh& mesh) {
	mesh.data.push_back({{1.f, -1.f, -1.f}, {0.f, 1.f}});
	mesh.data.push_back({{1.f, -1.f, 1.f}, {1.f, 1.f}});
	mesh.data.push_back({{1.f, 1.f, -1.f}, {0.f, 0.f}});

	mesh.data.push_back({{1.f, -1.f, 1.f}, {1.f, 1.f}});
	mesh.data.push_back({{1.f, 1.f, -1.f}, {0.f, 0.f}});
	mesh.data.push_back({{1.f, 1.f, 1.f}, {1.f, 0.f}});

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, mesh.data.size() * sizeof(game::simple_vertex), mesh.data.data(), GL_STATIC_DRAW);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(game::simple_vertex),  reinterpret_cast<void*>(0));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(game::simple_vertex),  reinterpret_cast<void*>(sizeof(float) * 3));

	mesh.vao = vao;
	mesh.vbo = vbo;
}

void generate_cube_sphere(dcon::data_container& state,  game::map_state& data, int world_size, bool fake) {
	push_face_vertices(
		state, data, world_size,
		cubeworld::back::origin,
		cubeworld::back::ds,
		cubeworld::back::dt,
		cubeworld::back::face, fake
	);
	push_face_vertices(
		state, data, world_size,
		cubeworld::forward::origin,
		cubeworld::forward::ds,
		cubeworld::forward::dt,
		cubeworld::forward::face, fake
	);
	push_face_vertices(
		state, data, world_size,
		cubeworld::left::origin,
		cubeworld::left::ds,
		cubeworld::left::dt,
		cubeworld::left::face, fake
	);
	push_face_vertices(
		state, data, world_size,
		cubeworld::right::origin,
		cubeworld::right::ds,
		cubeworld::right::dt,
		cubeworld::right::face, fake
	);
	push_face_vertices(
		state, data, world_size,
		cubeworld::top::origin,
		cubeworld::top::ds,
		cubeworld::top::dt,
		cubeworld::top::face, fake
	);
	push_face_vertices(
		state, data, world_size,
		cubeworld::bottom::origin,
		cubeworld::bottom::ds,
		cubeworld::bottom::dt,
		cubeworld::bottom::face, fake
	);

	auto& mesh = data.mesh.data;

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, mesh.size() * sizeof(game::vertex), mesh.data(), GL_STATIC_DRAW);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(game::vertex),  reinterpret_cast<void*>(0));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(game::vertex),  reinterpret_cast<void*>(sizeof(float) * 3));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(game::vertex),  reinterpret_cast<void*>(sizeof(float) * 3 + sizeof(float) * 3));

	glEnableVertexAttribArray(3);
	glVertexAttribIPointer(3, 1, GL_INT, sizeof(game::vertex),  reinterpret_cast<void*>(sizeof(float) * 3 + sizeof(float) * 3 + sizeof(float) * 2));

	data.mesh.vao = vao;
	data.mesh.vbo = vbo;
}

namespace geometry {

namespace screen_relative {
struct point {
	glm::vec2 data;
};
}

namespace world{
struct point {
	glm::vec2 data;
};
}

namespace screen_opengl {
struct point {
	glm::vec2 data;
};

point convert(const geometry::world::point in, const geometry::world::point camera, const float aspect_ratio, const float zoom) {
	auto result = (in.data - camera.data) * glm::vec2 { aspect_ratio, 1.f } * zoom;
	return {
		result
	};
}
}

}


bool color_is_land(uint8_t r, uint8_t g, uint8_t b) {
	if (r == 30 && g == 125 && b == 255) {
		return false;
	}
	if (r == 15 && g == 239 && b == 255) {
		return false;
	}
	if (r == 2 && g == 8 && b == 209) {
		return false;
	}
	return true;
}

bool color_is_fresh(uint8_t r, uint8_t g, uint8_t b) {
	if (r == 15 && g == 239 && b == 255) {
		return true;
	}
	return false;
}

bool equals(uint8_t r, uint8_t g, uint8_t b, uint8_t r1, uint8_t g1, uint8_t b1) {
	return r == r1 && g == g1 && b == b1;
}

float color_waterflow(uint8_t r, uint8_t g, uint8_t b) {
	if (r == 30 && g == 125 && b == 255) {
		return 0.f;
	}
	if (r == 15 && g == 239 && b == 255) {
		return 0.f;
	}
	if (r == 2 && g == 8 && b == 209) {
		return 0.f;
	}
	if (equals(r, g, b, 129, 9, 9)) {
		return 0.f;
	}
	if (equals(r, g, b, 244, 17, 17)) {
		return 800;
	}
	if (equals(r, g, b, 255, 132, 17)) {
		return 2000;
	}
	if (equals(r, g, b, 250, 250, 10)) {
		return 5259;
	}
	if (equals(r, g, b, 28, 255, 122)) {
		return 11250;
	}
	if (equals(r, g, b, 15, 175, 255)) {
		return 20000;
	}
	if (equals(r, g, b, 24, 77, 249)) {
		return 30000;
	}
	return 0.f;
}

struct shader_2d_data {
	GLuint shift;
	GLuint zoom;
	GLuint aspect_ratio;
};

static game::state world {};
static game::text_collection game_text {};

dcon::data_container state {};


uint32_t new_text(dcon::data_container& state, game::text_collection& collection, int32_t text_len, const char* data) {
	auto new_start = collection.text.size();
	auto key = collection.available_key;
	collection.text.resize(collection.text.size() + text_len + 1);
	std::copy(data, data + text_len, collection.text.data() + new_start);
	collection.word_start.push_back(new_start);
	collection.word_length.push_back(text_len);
	collection.associated_texture.push_back(0);
	collection.associated_texture_height.push_back(0);
	collection.associated_texture_width.push_back(0);
	collection.flags.push_back(0);
	collection.available_key++;
	return key;
}

void load_texture(game::text_collection& collection, uint32_t text_key) {
	if (collection.flags[text_key] & game::TEXT_KEY_IS_TEXTURE_PATH) {
		// already loaded
		return;
	}
	collection.flags[text_key] |= game::TEXT_KEY_IS_TEXTURE_PATH;

	std::string string_key { collection.text.data() + collection.word_start[text_key] };

	auto found = collection.existing_asset.find(string_key);

	if (found == collection.existing_asset.end()) {
		uint8_t * img;
		int width, height, channels;

		img = stbi_load(
			collection.text.data() + collection.word_start[text_key],
			&width,
			&height,
			&channels,
			4
		);

		glGenTextures(1, &collection.associated_texture[text_key]);
		glBindTexture(GL_TEXTURE_2D, collection.associated_texture[text_key]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA,
			width,
			height,
			0,
			GL_RGBA, GL_UNSIGNED_BYTE, img
		);

		assert_no_errors();

		collection.existing_asset[string_key] = text_key;
		collection.associated_texture_height[text_key] = height;
		collection.associated_texture_width[text_key] = width;
	} else {
		collection.associated_texture[text_key] = collection.associated_texture[found->second];
		collection.associated_texture_height[text_key] = collection.associated_texture_height[found->second];
		collection.associated_texture_width[text_key] = collection.associated_texture_width[found->second];
	}
}


extern "C" {
	DCON_LUADLL_API uint32_t register_text(int32_t text_len, const char* data);
	DCON_LUADLL_API uint32_t register_texture(int32_t text_len, const char* data);
};
uint32_t register_text(int32_t text_len, const char* data) {
	return new_text(state, game_text, text_len, data);
}
uint32_t register_texture(int32_t text_len, const char* data) {
	auto text_key =  new_text(state, game_text, text_len, data);
	load_texture(game_text, text_key);
	return text_key;
}

extern "C" {
	uint32_t age_years(dcon::pop_id pop);
}

uint8_t age_bracket(dcon::data_container& state, dcon::race_id race, uint32_t age) {
	if (age < state.race_get_child_age(race)) {
		return 0;
	}
	if (age < state.race_get_teen_age(race)) {
		return 1;
	}
	if (age < state.race_get_adult_age(race)) {
		return 2;
	}
	if (age < state.race_get_middle_age(race)) {
		return 3;
	}
	if (age < state.race_get_elder_age(race)) {
		return 4;
	}
	return 5;
}

void render_portrait(dcon::data_container& state, game::text_collection& assets, dcon::pop_id pop, int vertices, GLuint uv_mod_location) {
	auto female = state.pop_get_female(pop);
	auto race = state.pop_get_race(pop);
	auto portrait =
		female
		? state.race_get_portrait_fallback_female(race)
		: state.race_get_portrait_fallback_male(race);

	auto age = age_years(pop);
	auto bracket = age_bracket(state, race, age);
	auto portrait_from_age =
		female
		? state.race_get_portrait_per_age_bracket_female(race, bracket)
		: state.race_get_portrait_per_age_bracket_male(race, bracket);

	if (portrait_from_age) {
		portrait = portrait_from_age;
	}

	if (!portrait) {
		return;
	}

	std::vector<float> dna_per_layer;
	for (uint32_t i = 0; i < state.portrait_set_get_layers_size(); i++) {
		if(!state.portrait_set_get_layers(portrait, i)) break;
		dna_per_layer.push_back(state.pop_get_dna(pop, i));
	}

	for (uint8_t i = 0; i < state.portrait_set_get_groups_size(); i++) {
		auto group = state.portrait_set_get_groups(portrait, i);
		if (!group) break;
		auto first_layer = group.get_group(0);
		// find index of this layer:
		uint32_t dna_index_of_the_first_layer = 0;
		for (uint32_t k = 0; k < state.portrait_set_get_layers_size(); k++) {
			if(!state.portrait_set_get_layers(portrait, k)) break;
			if(state.portrait_set_get_layers(portrait, k) == first_layer) {
				dna_index_of_the_first_layer = k;
				break;
			}
		}
		for (uint8_t j = 0; j < state.portrait_layer_group_get_group_size(); j++) {
			auto grouped_layer = state.portrait_layer_group_get_group(group, j);
			uint32_t dna_index_of_the_current_layer = 0;
			for (uint32_t k = 0; k < state.portrait_set_get_layers_size(); k++) {
				if(!state.portrait_set_get_layers(portrait, k)) break;
				if(state.portrait_set_get_layers(portrait, k) == grouped_layer) {
					dna_index_of_the_current_layer = k;
					break;
				}
			}
			dna_per_layer[dna_index_of_the_current_layer] = dna_per_layer[dna_index_of_the_first_layer];
		}
	}

	for (uint32_t i = 0; i < state.portrait_set_get_layers_size(); i++) {
		auto layer = state.portrait_set_get_layers(portrait, i);
		if(!layer) break;
		auto dna = dna_per_layer[i];
		auto asset = state.portrait_layer_get_path_text_index(layer);

		auto frames = assets.associated_texture_width[asset] / assets.associated_texture_height[asset];
		int frame_index = (int)(dna * frames);
		auto frame_step = 1.f / (float) frames;
		float texture_start = frame_step * (float)(frame_index);
		float texture_end = frame_step * (float)(frame_index + 1);
		glUniform4f(uv_mod_location, texture_start, 0.f, frame_step, 1.f);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, assets.associated_texture[asset]);

		glDrawArrays(
			GL_TRIANGLES,
			0,
			vertices
		);
	}
}

open_project_t bytes_to_project(serialization::in_buffer& buffer);

static simple_fs::file_system common_fs {};
static open_project_t example_ui_project {};
static template_project::project ui_templates {};
static asvg::file_bank svg_image_files {};

void handle_main_menu(
	ogl::data& ogl_state, text::font_manager& font_collection,
	window_element_wrapper_t& win,
	window_element_data_container_t& win_instance,
	int width, int height,
	mouse_probe& probe,
	uint32_t bg_key, float ui_scale
) {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glViewport(0, 0, (int)width, (int)height);
	float aspect_ratio = (float)width / (float)height;
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	use_program(width, height);

	auto bg_width = (float)game_text.associated_texture_width[bg_key];
	auto bg_height = (float)game_text.associated_texture_height[bg_key];
	auto scale = (float) height / bg_height;

	auto x_bg = (float)width / 2 - bg_width / 2.f * scale;

	render_textured_rect(
		{1.f, 1.f, 1.f},
		x_bg, 0,
		bg_width * scale, bg_height * scale,
		game_text.associated_texture[bg_key]
	);

	auto x = (float)width / 2.f - win.wrapped.x_size * ui_scale / 2.f;
	auto y = (float)height / 2.f - win.wrapped.y_size * ui_scale / 2.f;

	render_window(
		ogl_state,
		common_fs,
		font_collection,
		svg_image_files,
		example_ui_project,
		ui_templates,
		win, win_instance,
		x / ui_scale, y / ui_scale, width, height, probe,
		false,
		ui_scale
	);

	assert_no_errors();
}

void lighting_settings(ImGuiIO& io, glm::vec3& light_direction, glm::vec3& ambient) {
	static float f = 0.0f;
	static int counter = 0;

	ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

	ImGui::Text("Test.");               // Display some text (you can use a format strings too)

	ImGui::SliderFloat3("float", (float*)&light_direction, -1.0f, 1.0f);
	light_direction = glm::normalize(light_direction);

	ImGui::ColorEdit3("clear color", (float*)&ambient); // Edit 3 floats representing a color

	if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
		counter++;
	ImGui::SameLine();
	ImGui::Text("counter = %d", counter);

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
	ImGui::End();
}

void race_explorer(const float TEXT_BASE_HEIGHT) {
	ImGui::Begin("Races");

	// Create item list
	static std::vector<dcon::race_id> items;
	if (items.size() == 0) {
		state.for_each_race([&](auto race_id){
			items.push_back(race_id);
		});
	}

	// Options
	static ImGuiTableFlags flags =
	ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
	| ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody
	| ImGuiTableFlags_ScrollY;

	if (
		ImGui::BeginTable(
			"table_sorting",
			5,
			flags,
			ImVec2(0.0f, TEXT_BASE_HEIGHT * 15),
			0.0f)
	) {

		ImGui::TableSetupColumn(
			"Icon",
			ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed,
			20.0f,
			int(race_table_columns_id::icon)
		);

		ImGui::TableSetupColumn(
			"ID",
			ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed,
			60.0f,
			int(race_table_columns_id::race_id)
		);

		ImGui::TableSetupColumn(
			"Name",
			ImGuiTableColumnFlags_WidthFixed,
			120.0f,
			int(race_table_columns_id::name)
		);

		ImGui::TableSetupColumn(
			"Max age",
			ImGuiTableColumnFlags_WidthFixed,
			0.0f,
			int(race_table_columns_id::max_age)
		);

		ImGui::TableSetupScrollFreeze(0, 1); // Make row always visible
		ImGui::TableHeadersRow();

		// Sort our data if sort specs have been changed!
		if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
			if (sort_specs->SpecsDirty) {
				std::sort(items.begin(), items.end(), [&](dcon::race_id a, dcon::race_id b){
					for (int n = 0; n < sort_specs->SpecsCount; n++) {
						auto& sorted_colum = sort_specs->Specs[n];
						auto column = race_table_columns_id(sorted_colum.ColumnUserID);
						switch (column) {
						case race_table_columns_id::race_id:
						case race_table_columns_id::icon:
							return a.index() < b.index();
						case race_table_columns_id::
							name:
						{
							auto key_a = state.race_get_name_text_index(a);
							auto key_b = state.race_get_name_text_index(b);
							return std::strcmp(
								game_text.text.data() + game_text.word_start[key_a],
								game_text.text.data() + game_text.word_start[key_b]
							) < 0;
						}
						case race_table_columns_id::
							males_per_hundred_females:
							return
								state.race_get_males_per_hundred_females(a)
								<
								state.race_get_males_per_hundred_females(b);
						case race_table_columns_id::
							child_age:
							return
								state.race_get_males_per_hundred_females(a)
								<
								state.race_get_males_per_hundred_females(b);
						case race_table_columns_id::
							teen_age:
							return
								state.race_get_males_per_hundred_females(a)
								<
								state.race_get_males_per_hundred_females(b);
						case race_table_columns_id::
							middle_age:
							return
								state.race_get_males_per_hundred_females(a)
								<
								state.race_get_males_per_hundred_females(b);
						case race_table_columns_id::
							elder_age:
							return
								state.race_get_males_per_hundred_females(a)
								<
								state.race_get_males_per_hundred_females(b);
						case race_table_columns_id::
							max_age:
							return
								state.race_get_max_age(a)
								<
								state.race_get_max_age(b);
						case race_table_columns_id::
							minimum_comfortable_temperature:
							return
								state.race_get_max_age(a)
								<
								state.race_get_max_age(b);
						case race_table_columns_id::
							female_body_size:
							return
								state.race_get_female_body_size(a)
								<
								state.race_get_female_body_size(b);
						case race_table_columns_id::
							male_body_size:
							return
								state.race_get_male_body_size(a)
								<
								state.race_get_male_body_size(b);
						break;
						default:
							assert(false);
							fprintf(stderr, "Unknown race enum value");
							exit(1);
						}
					}
					return a.index() < b.index();
				});
				sort_specs->SpecsDirty = false;
			}
		}

		// Demonstrate using clipper for large vertical lists
		ImGuiListClipper clipper;
		clipper.Begin(items.size());
		while (clipper.Step()) {
			for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++) {
				// Display a data item
				auto item = items[row_n];
				ImGui::PushID(item.index());

				ImGui::TableNextRow();


				ImGui::TableNextColumn();
				auto texture_id = game_text.associated_texture[state.race_get_icon_path_text_index(item)];
				ImGui::ImageWithBg(
					texture_id,
					{20.f, 20.f},
					{0, 0},
					{1, 1},
					{
						1.f,
						1.f,
						1.f,
						0.f
					},
					{
						state.race_get_r(item),
						state.race_get_g(item),
						state.race_get_b(item),
						1.f
					}
				);

				ImGui::TableNextColumn();
				ImGui::Text("%04d", item.index());

				ImGui::TableNextColumn();
				auto name_text_index = state.race_get_name_text_index(item);
				ImGui::TextUnformatted(game_text.text.data() + game_text.word_start[name_text_index]);

				ImGui::TableNextColumn();
				ImGui::Text("%f", state.race_get_max_age(item));

				ImGui::PopID();
			}
		}
		ImGui::EndTable();
	}

	ImGui::End();
}

template <typename F>
void map_modes(std::vector<uint8_t>& map_mode_data, int world_size, F&& commit_map_mode) {
	ImGui::Begin("Map mode");
	const char* items[] = {
		"White",
		"Coast",
		"Plates",
		"Waterflow (Winter)",
		"Waterflow (Summer)",
		"Land",
		"Heightmap",
		"Soil organics",
		"Ice",
		"Rocks",
		"Biomes"
	};

	static int item_selected_idx = 0;
	static bool requested_map_update = false;
	static int loaded_idx = -1;
	static bool item_highlight = false;

	// Custom size: use all width, 5 items tall
	ImGui::Text("Full-width:");
	if (ImGui::BeginListBox(
		"##mapmode_listbox",
		ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())
	)) {
		for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
			bool is_selected = (item_selected_idx == n);
			if (ImGui::Selectable(items[n], is_selected, 0))
				item_selected_idx = n;
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndListBox();
	}

	static bool ice_age = false;
	if (item_selected_idx == 8) {
		if (ImGui::Checkbox("Ice age?", &ice_age)) {
			requested_map_update = true;
		}
	}

	ImGui::End();


	if (item_selected_idx != loaded_idx || requested_map_update) {
		loaded_idx = item_selected_idx;
		requested_map_update = false;
		if (item_selected_idx == 0) {
			state.for_each_tile([&](dcon::tile_id tile) {
				auto fst = tile_to_fst(world_size, tile);
				auto index = fst.x * world_size * world_size + fst.z * world_size + fst.y;
				map_mode_data[4 * index + 0] = 255;
				map_mode_data[4 * index + 1] = 255;
				map_mode_data[4 * index + 2] = 255;
				map_mode_data[4 * index + 3] = 255;
			});
		} else if (item_selected_idx == 1) {
			state.for_each_tile([&](dcon::tile_id tile) {
				auto fst = tile_to_fst(world_size, tile);
				auto index = fst.x * world_size * world_size + fst.z * world_size + fst.y;
				if (state.tile_get_is_coast(tile)) {
					map_mode_data[4 * index + 0] = 0;
					map_mode_data[4 * index + 1] = 0;
					map_mode_data[4 * index + 2] = 0;
					map_mode_data[4 * index + 3] = 255;
				} else {
					map_mode_data[4 * index + 0] = 255;
					map_mode_data[4 * index + 1] = 255;
					map_mode_data[4 * index + 2] = 255;
					map_mode_data[4 * index + 3] = 255;
				}
			});
		} else if (item_selected_idx == 2) {
			state.for_each_tile([&](dcon::tile_id tile) {
				auto plate = state.tile_get_plate_from_plate_tiles(tile);
				auto fst = tile_to_fst(world_size, tile);
				auto r = state.plate_get_r(plate);
				auto g = state.plate_get_g(plate);
				auto b = state.plate_get_b(plate);
				auto index = fst.x * world_size * world_size + fst.z * world_size + fst.y;
				map_mode_data[4 * index + 0] = (uint8_t)(r * 255);
				map_mode_data[4 * index + 1] = (uint8_t)(g * 255);
				map_mode_data[4 * index + 2] = (uint8_t)(b * 255);
				map_mode_data[4 * index + 3] = 255;
			});
		} else if (item_selected_idx == 3) {
			state.for_each_tile([&](dcon::tile_id tile) {
				auto fst = tile_to_fst(world_size, tile);
				auto index = fst.x * world_size * world_size + fst.z * world_size + fst.y;
				auto waterflow = state.tile_get_january_waterflow(tile);
				map_mode_data[4 * index + 0] = (255 - (uint8_t)(waterflow / 20000.f * 255)) / 10;
				map_mode_data[4 * index + 1] = 0;
				map_mode_data[4 * index + 2] = (uint8_t)(waterflow / 20000.f * 255);
				map_mode_data[4 * index + 3] = 255;
			});
		} else if (item_selected_idx == 4) {
			state.for_each_tile([&](dcon::tile_id tile) {
				auto fst = tile_to_fst(world_size, tile);
				auto index = fst.x * world_size * world_size + fst.z * world_size + fst.y;
				auto waterflow = state.tile_get_july_waterflow(tile);
				map_mode_data[4 * index + 0] = (255 - (uint8_t)(waterflow / 20000.f * 255)) / 10;
				map_mode_data[4 * index + 1] = 0;
				map_mode_data[4 * index + 2] = (uint8_t)(waterflow / 20000.f * 255);
				map_mode_data[4 * index + 3] = 255;
			});
		} else if (item_selected_idx == 5) {
			state.for_each_tile([&](dcon::tile_id tile) {
				auto fst = tile_to_fst(world_size, tile);
				auto index = fst.x * world_size * world_size + fst.z * world_size + fst.y;
				if (!state.tile_get_is_land(tile)) {
					map_mode_data[4 * index + 0] = 0;
					map_mode_data[4 * index + 1] = 0;
					map_mode_data[4 * index + 2] = 0;
					map_mode_data[4 * index + 3] = 255;
				} else {
					map_mode_data[4 * index + 0] = 255;
					map_mode_data[4 * index + 1] = 255;
					map_mode_data[4 * index + 2] = 255;
					map_mode_data[4 * index + 3] = 255;
				}
			});
		} else if (item_selected_idx == 6) {
			state.for_each_tile([&](dcon::tile_id tile) {
				auto fst = tile_to_fst(world_size, tile);
				auto index = fst.x * world_size * world_size + fst.z * world_size + fst.y;
				auto elevation = state.tile_get_elevation(tile);
				auto score = (uint8_t)((elevation / 16000.f + 0.5f) * 255.f);
				map_mode_data[4 * index + 0] = score;
				map_mode_data[4 * index + 1] = score;
				map_mode_data[4 * index + 2] = score;
				map_mode_data[4 * index + 3] = 255;
			});
		} else if (item_selected_idx == 7) {
			state.for_each_tile([&](dcon::tile_id tile) {
				auto fst = tile_to_fst(world_size, tile);
				auto index = fst.x * world_size * world_size + fst.z * world_size + fst.y;
				auto soil_organics = state.tile_get_soil_organics(tile);
				auto score = (uint8_t)(soil_organics * 255.f);
				map_mode_data[4 * index + 0] = score;
				map_mode_data[4 * index + 1] = score;
				map_mode_data[4 * index + 2] = score;
				map_mode_data[4 * index + 3] = 255;
			});
		} else if (item_selected_idx == 8) {
			state.for_each_tile([&](dcon::tile_id tile) {
				auto fst = tile_to_fst(world_size, tile);
				auto index = fst.x * world_size * world_size + fst.z * world_size + fst.y;
				auto ice = state.tile_get_ice(tile);
				if (ice_age) {
					ice = state.tile_get_ice_age_ice(tile);
				}
				auto score = (uint8_t)(ice * 6.f);
				map_mode_data[4 * index + 0] = score;
				map_mode_data[4 * index + 1] = score;
				map_mode_data[4 * index + 2] = score;
				map_mode_data[4 * index + 3] = 255;
			});
		} else if (item_selected_idx == 9) {
			state.for_each_tile([&](dcon::tile_id tile) {
				auto rock = state.tile_get_bedrock(tile);
				auto fst = tile_to_fst(world_size, tile);
				auto index = fst.x * world_size * world_size + fst.z * world_size + fst.y;
				auto r = state.bedrock_get_r(rock);
				auto g = state.bedrock_get_g(rock);
				auto b = state.bedrock_get_b(rock);
				map_mode_data[4 * index + 0] = (uint8_t)(r * 255);
				map_mode_data[4 * index + 1] = (uint8_t)(g * 255);
				map_mode_data[4 * index + 2] = (uint8_t)(b * 255);
				map_mode_data[4 * index + 3] = 255;
			});
		} else if (item_selected_idx == 10) {
			state.for_each_tile([&](dcon::tile_id tile) {
				auto biome = state.tile_get_biome(tile);
				auto fst = tile_to_fst(world_size, tile);
				auto index = fst.x * world_size * world_size + fst.z * world_size + fst.y;
				auto r = state.biome_get_r(biome);
				auto g = state.biome_get_g(biome);
				auto b = state.biome_get_b(biome);
				map_mode_data[4 * index + 0] = (uint8_t)(r * 255);
				map_mode_data[4 * index + 1] = (uint8_t)(g * 255);
				map_mode_data[4 * index + 2] = (uint8_t)(b * 255);
				map_mode_data[4 * index + 3] = 255;
			});
		}
		commit_map_mode();
	}
}

struct shadow_shader {
	GLuint program;
	GLuint model;
	GLuint view;
};

struct planet_data_shader {
	GLuint program;
	GLuint model;
	GLuint view;
	GLuint projection;
	GLuint light_direction;
	GLuint light_color;
	GLuint ambient_color;
	GLuint albedo_color;
	GLuint camera_position;
	GLuint map_data;
	GLuint shadow_map;
	GLuint shadow_layers;
	GLuint is_sky;
	GLuint shadow_projection;
};

struct world_rendering_data {
	int shadow_layers;
	std::vector<GLuint> shadow_fbo {};
	std::vector<GLuint> shadow_renderbuffers {};
	int shadow_map_resolution;
	shadow_shader shadow_shader;
	planet_data_shader data_shader;
	GLuint shadow_map_texture;
	GLuint map_mode_texture;

	glm::vec3 albedo_world;
};

struct camera_data {
	glm::mat4 view;
	glm::vec3 position;
	glm::vec2 speed;
	glm::vec3 eye;
	glm::mat4 projection;
	float far_plane;
	float near_plane;
};

void update_camera(
	camera_data& camera,
	int world_size,
	float dt,
	float width,
	float height
) {
	glm::vec3 eye {
		cosf(camera.position.y) * sinf(camera.position.x),
		sinf(camera.position.y),
		cosf(camera.position.y) * cosf(camera.position.x)
	};
	eye *=  camera.position.z;

	float target_zoom = 1.5;
	float speed_mult = 20.f;
	if (desired_zoom_level == -2) {
		auto tile = r3_to_tile(world_size, eye);
		auto elevation = state.tile_get_elevation(tile);
		auto zoom_adjustment = opengl_elevation(elevation);
		target_zoom = zoom_adjustment + 0.05f;
		speed_mult = 5.f;
	}
	if (desired_zoom_level == -1) {
		auto tile = r3_to_tile(world_size, eye);
		auto elevation = state.tile_get_elevation(tile);
		auto zoom_adjustment = opengl_elevation(elevation);
		target_zoom = zoom_adjustment + 0.2f;
		speed_mult = 10.f;
	}
	if (desired_zoom_level == 1) {
		target_zoom = 2;
		speed_mult = 40.f;
	}

	camera.speed *= exp(-dt * 10.f);
	camera.speed += glm::vec2(float(current_move_x), float(current_move_y)) * dt;

	auto zoom_direction = target_zoom - camera.position.z;
	camera.position.xy += camera.speed * dt * speed_mult;
	camera.position.z += zoom_direction * dt * 2.f;

	camera.position.y = std::clamp(
		camera.position.y,
		-glm::pi<float>() / 2.f * 0.95f,
		glm::pi<float>() / 2.f * 0.95f
	);

	float near_plane = 0.1f;
	float far_plane = camera.position.z * 1.2f;
	if (desired_zoom_level == -2) {
		near_plane = 0.01f;
		far_plane = camera.position.z * 1.2f;
	}
	if (desired_zoom_level == -1) {
		near_plane = 0.01f;
		far_plane = camera.position.z * 1.2f;
	}
	if (desired_zoom_level == 0) {
		near_plane = 0.1f;
		float far_plane = 0.6f;
	}

	camera.view = glm::lookAt(
		camera.eye,
		{0.f, 0.f, 0.f},
		{0.f, 1.f, 0.f}
	);

	camera.projection = glm::perspective(
		glm::pi<float>() / 3.f, width / height, near_plane, far_plane
	);
}

void render_world(
	GLFWwindow* window,
	world_rendering_data& rendering_data,
	camera_data& camera,
	std::vector<uint8_t>& map_mode_data,
	const glm::vec3& light_direction,
	const glm::vec3& ambient_color,
	const glm::vec3& light_color,
	float width,
	float height
) {
	glm::vec3 light_z = glm::normalize(light_direction);
	glm::vec3 light_x = glm::normalize(glm::cross(light_z, {0.f, 0.f, 1.f}));
	glm::vec3 light_y = glm::cross(light_x, light_z);

	// drawing shadow maps
	std::vector<glm::mat4> shadow_projections;

	assert(rendering_data.shadow_layers == rendering_data.shadow_fbo.size());
	assert(rendering_data.shadow_layers == rendering_data.shadow_renderbuffers.size());
	auto shadow_layers = rendering_data.shadow_layers;

	for (GLsizei i = 0; i < shadow_layers; i++) {
		float ratio = camera.far_plane / camera.near_plane;
		float current_layer_ratio = (float) i / shadow_layers;
		float frustum_split_near = camera.near_plane * pow(ratio, current_layer_ratio);
		float next_layer_ratio = (float) (i + 1) / shadow_layers;
		float power = pow(ratio, next_layer_ratio);
		float frustum_split_far = camera.near_plane * power;

		glm::mat4 projection_shadow_range = glm::perspective(
			glm::pi<float>() / 3.f, width / height, frustum_split_near, frustum_split_far
		);

		auto visible_world = frustum(projection_shadow_range * camera.view).vertices;

		/*
		std::vector<glm::vec3> visible_world {
			{-2.f, -2.f, -2.f},
			{-2.f, -2.f, +2.f},
			{-2.f, +2.f, -2.f},
			{+2.f, -2.f, -2.f},
			{-2.f, +2.f, +2.f},
			{+2.f, -2.f, +2.f},
			{+2.f, +2.f, -2.f},
			{+2.f, +2.f, +2.f},
			{-2.f, -2.f, -2.f},
		};
		*/

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rendering_data.shadow_fbo[i]);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		glDisable(GL_CULL_FACE);

		glDisable(GL_BLEND);

		glClearColor(1.0f, 1.0f, 0.0f, 0.0f);
		glClearDepth(1.0f);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, rendering_data.shadow_map_resolution, rendering_data.shadow_map_resolution);

		// projection of corners on X
		float min_x = std::numeric_limits<float>::max();
		float max_x = -std::numeric_limits<float>::max();
		for (auto& corner : visible_world) {
			max_x = std::max(max_x, glm::dot(corner, light_x));
			min_x = std::min(min_x, glm::dot(corner, light_x));
		}

		// projection of corners on Y
		float min_y = std::numeric_limits<float>::max();
		float max_y = -std::numeric_limits<float>::max();
		for (auto& corner : visible_world) {
			max_y = std::max(max_y, glm::dot(corner, light_y));
			min_y = std::min(min_y, glm::dot(corner, light_y));
		}

		// projection of corners on Z
		float min_z = std::numeric_limits<float>::max();
		float max_z = -std::numeric_limits<float>::max();
		for (auto& corner : visible_world) {
			max_z = std::max(max_z, glm::dot(corner, light_z));
			min_z = std::min(min_z, glm::dot(corner, light_z));
		}

		glm::vec3 true_center {0.f};
		for (auto& corner : visible_world) {
			true_center += corner;
		}
		true_center /= (float)(visible_world.size());

/*
		auto corner = ground.model.meshes[0].max + glm::vec3(0.0, 10.0, 0.0);
		max_z = std::max(max_z, glm::dot(corner, light_z));
		min_z = std::min(min_z, glm::dot(corner, light_z));

		corner = ground.model.meshes[0].min;
		max_z = std::max(max_z, glm::dot(corner, light_z));
		min_z = std::min(min_z, glm::dot(corner, light_z));
*/

		glm::vec3 max = {max_x, max_y, max_z};
		glm::vec3 min = {min_x, min_y, min_z};

		auto center = (max + min) * 0.5f;

		glm::vec3 to_min {};//min - center;
		glm::vec3 to_max {};//max - center;



		auto center_world = glm::mat3(light_x, light_y, light_z) * center;

		auto ortho = glm::ortho(
			min_x, max_x,
			min_y, max_y,
			-max_z, -min_z
		);
		auto basis_change = glm::mat4(glm::transpose(glm::mat3(light_x, light_y, light_z)));

		glm::mat4 light_projection = ortho * basis_change;
		auto new_basis_center = basis_change * glm::vec4{true_center, 1.f};
		auto image_of_center = light_projection * glm::vec4{true_center, 1.f};
		auto projection_of_center = glm::dot(light_z, true_center);

		shadow_projections.push_back(light_projection);

		// projection_full_range = light_projection;

		glUseProgram(rendering_data.shadow_shader.program);
		glm::mat4 model (1.f);
		glUniformMatrix4fv(rendering_data.shadow_shader.model, 1, GL_FALSE, reinterpret_cast<float *>(&model));
		glUniformMatrix4fv(rendering_data.shadow_shader.view, 1, GL_FALSE, reinterpret_cast<float *>(&light_projection));

		glBindVertexArray(world.map.mesh.vao);
		glDrawArrays(
			GL_TRIANGLES,
			0,
			world.map.mesh.data.size()
		);
	}

	assert_no_errors();

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glViewport(0, 0, (int)width, (int)height);
	float aspect_ratio = width / height;
	glClearColor(ambient_color.x, ambient_color.y, ambient_color.z, 0.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// glDisable(GL_CULL_FACE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(rendering_data.data_shader.program);

	glActiveTexture(GL_TEXTURE10);
	glBindTexture(GL_TEXTURE_2D_ARRAY, rendering_data.shadow_map_texture);
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, rendering_data.map_mode_texture);

	glm::mat4 model (1.f);

	auto& planet_shader = rendering_data.data_shader;
	glUniformMatrix4fv(rendering_data.data_shader.model, 1, GL_FALSE, reinterpret_cast<float *>(&model));
	glUniformMatrix4fv(rendering_data.data_shader.view, 1, GL_FALSE, reinterpret_cast<float *>(&camera.view));
	glUniformMatrix4fv(rendering_data.data_shader.projection, 1, GL_FALSE, reinterpret_cast<float *>(&camera.projection));
	glUniform3fv(planet_shader.light_direction, 1, reinterpret_cast<const float *>(&light_direction));
	glUniform3fv(planet_shader.light_color, 1,  reinterpret_cast<const float *>(&light_color));
	glUniform3fv(planet_shader.ambient_color, 1,  reinterpret_cast<const float *>(&ambient_color));
	glUniform3fv(planet_shader.albedo_color, 1, reinterpret_cast<const float *>(&rendering_data.albedo_world));
	glUniform3fv(planet_shader.camera_position, 1, reinterpret_cast<const float *>(&camera.eye));
	glUniform1i(planet_shader.map_data, 0);
	glUniform1i(planet_shader.shadow_map, 10);
	glUniform1i(planet_shader.shadow_layers, shadow_layers);
	glUniform1i(planet_shader.is_sky, 0);
	glUniformMatrix4fv(planet_shader.shadow_projection, shadow_layers, GL_FALSE, reinterpret_cast<float *>(shadow_projections.data()));
	assert_no_errors();
	glBindVertexArray(world.map.mesh.vao);
	glDrawArrays(
		GL_TRIANGLES,
		0,
		world.map.mesh.data.size()
	);


	glCullFace(GL_FRONT);
	glUniform1i(planet_shader.is_sky, 1);
	glBindVertexArray(world.sky.mesh.vao);
	glDrawArrays(
		GL_TRIANGLES,
		0,
		world.sky.mesh.data.size()
	);

	assert_no_errors();
}

struct textured_rectangle_shader {
	GLuint program;
	GLuint view;
	GLuint projection;
	GLuint model;
	GLuint texture_unit;
	GLuint uv_map;
};

void render_characters(
	textured_rectangle_shader shader,
	game::simple_mesh square
) {
	glDisable(GL_DEPTH_TEST);
	glUseProgram(shader.program);
	glDisable(GL_CULL_FACE);
	glUniformMatrix4fv(shader.view, 1, GL_FALSE, reinterpret_cast<float *>(&shader.view));
	glUniformMatrix4fv(shader.projection, 1, GL_FALSE, reinterpret_cast<float *>(&shader.projection));
	glUniform1i(shader.texture_unit, 0);
	glBindVertexArray(square.vao);

	state.for_each_settlement([&](auto settlement){
		auto tile = state.settlement_get_tile_from_settlement_tile(settlement);
		auto x = state.tile_get_x(tile);
		auto y = state.tile_get_y(tile);
		auto z = state.tile_get_z(tile);

		auto elevation = state.tile_get_elevation(tile);
		auto scale_r = opengl_elevation(elevation);

		auto rect = sphere_to_rect({x, y, z});
		rect.x = 0.5f - rect.x;
		rect.y = 1.f - rect.y;

		glm::mat4 model_square (1.f);
		model_square = glm::rotate(model_square, rect.x * glm::pi<float>() * 2.f, {0.f, 1.f, 0.f});
		model_square = glm::rotate(model_square, (rect.y - 0.5f) * glm::pi<float>(), {0.f, 0.f, 1.f});
		model_square = glm::scale(model_square, {scale_r * 1.001f, 0.0015f, 0.0015f});
		glUniformMatrix4fv(shader.model, 1, GL_FALSE, reinterpret_cast<float *>(&model_square));

		state.settlement_for_each_pop_location(settlement, [&](dcon::pop_location_id pop_location){
			auto pop = state.pop_location_get_pop(pop_location);
			render_portrait(state, game_text, pop, square.data.size(), shader.uv_map);
		});
		/*
		glDrawArrays(
			GL_TRIANGLES,
			0,
			square.data.size()
		);
		*/
	});

	assert_no_errors();
}

enum class game_scene {
	main_menu = 0,
	loading_images = 1,
	world_generation = 2,
	world_exploration = 3,
	game = 4,
	total = 5
};

game_scene current_scene = game_scene::main_menu;
bool settings_opened = false;

extern "C" {
	DCON_LUADLL_API void change_scene(uint8_t scene) {
		if (scene >= (uint8_t)game_scene::total) {
			window::emit_error_message("Invalid scene", false);
			return;
		}
		current_scene = (game_scene)(scene);
	}

	DCON_LUADLL_API void toggle_settings_window() {
		settings_opened = !settings_opened;
	}
}

void load_world_from_images(
	lua_State* L,
	std::vector<uint8_t>& map_mode_data,
	GLuint& map_mode_texture,
	int& world_size
) {
	int result;

	lua_pushcfunction(L, traceback);
	// [traceback

	lua_getfield(L, LUA_GLOBALSINDEX, "sote");
	// [traceback, sote

	lua_getfield(L, -1, "load_raws");
	// [traceback, sote, load_raws

	result = lua_pcall(L, 0, LUA_MULTRET, -3);
	// [traceback, sote

	if (result) exit(1);

	lua_pop(L, 1);
	// [traceback

	// get world size

	lua_getfield(L, LUA_GLOBALSINDEX, "DEFINES");
	// [traceback, DEFINES

	lua_getfield(L, -1, "world_size");
	// [traceback, DEFINES, world_size

	world_size = (int)(lua_tonumber(L, -1));
	// [traceback, DEFINES, world_size

	lua_pop(L, 2);
	// [traceback

	// load images

	// -- After we create the empty world, we can fill it with data...
	printf("Loading tectonics map...");

	{
		ankerl::unordered_dense::map<int32_t, dcon::plate_id> detected_plates{};

		std::string filename = "./lua/default/tectonics.png";
		uint8_t * img;
		int width, height, channels;

		img = stbi_load(
			filename.c_str(),
			&width,
			&height,
			&channels,
			4
		);

		state.for_each_tile([&](dcon::tile_id tile) {
			auto sphere = tile_to_sphere(world_size, tile);
			auto rect = sphere_to_rect(sphere);
			auto index = rect_to_image_index(width, height, rect);

			auto r = img[index * 4 + 0];
			auto g = img[index * 4 + 1];
			auto b = img[index * 4 + 2];

			auto cid = rgb_to_id(r, g, b);

			auto it = detected_plates.find(cid);

			if (it == detected_plates.end()) {
				auto new_plate = state.create_plate();
				state.plate_set_r(new_plate, (float)r / 255.f);
				state.plate_set_g(new_plate, (float)g / 255.f);
				state.plate_set_b(new_plate, (float)b / 255.f);
				state.plate_set_direction(new_plate, 1);
				detected_plates[cid] = new_plate;
				state.force_create_plate_tiles(new_plate, tile);
			} else {
				state.force_create_plate_tiles(it->second, tile);
			}
		});
	}

	printf("Tectonic map loaded!\n");

	printf("Generate tile neigbours\n");

	float scaler_world = 1.f / float(world_size);
	state.tile_resize_neighbour(4);

	state.for_each_tile([&](dcon::tile_id tile) {
		auto point = tile_to_sphere(world_size, tile);
		auto box = sphere_to_box(point);
		auto fst = tile_to_fst(world_size, tile);
		auto shift_s = face_to_ds[fst.x] * scaler_world;
		auto shift_t = face_to_dt[fst.x] * scaler_world;

		state.tile_set_x(tile, point.x);
		state.tile_set_y(tile, point.y);
		state.tile_set_z(tile, point.z);

		// 00 - forward
		{
			auto n = r3_to_tile(world_size, box + shift_s);
			state.tile_set_neighbour(tile, 0, n);
		}
		// 01 - left
		{
			auto n = r3_to_tile(world_size, box - shift_t);
			state.tile_set_neighbour(tile, 1, n);
		}
		// 10 - right
		{
			auto n = r3_to_tile(world_size, box + shift_t);
			state.tile_set_neighbour(tile, 2, n);
		}
		// 11 - backward
		{
			auto n = r3_to_tile(world_size, box - shift_s);
			state.tile_set_neighbour(tile, 3, n);

			auto n_p = tile_to_sphere(world_size, state.tile_get_neighbour(tile, 3));
			auto c_p = tile_to_sphere(world_size, tile);
			auto distance = glm::distance(c_p, n_p);
			assert(distance < scaler_world * 5.f);
		}

		assert(fst_to_tile(world_size, tile_to_fst(world_size, tile)) == tile);
		assert(sphere_to_fst(world_size, point) == fst);
		assert(glm::distance(point, fst_to_sphere(world_size, sphere_to_fst(world_size, point))) < scaler_world);
	});

	printf("Tile neigbours are generated\n");

	printf("Loading hydro maps\n");

	{
		std::string filename_january = "./lua/default/waterflow-january.png";
		std::string filename_july = "./lua/default/waterflow-july.png";
		uint8_t * january;
		uint8_t * july;
		int width, height, channels;

		january = stbi_load(
			filename_january.c_str(),
			&width,
			&height,
			&channels,
			4
		);
		july = stbi_load(
			filename_july.c_str(),
			&width,
			&height,
			&channels,
			4
		);

		state.for_each_tile([&](dcon::tile_id tile) {
			auto sphere = tile_to_sphere(world_size, tile);
			auto rect = sphere_to_rect(sphere);
			auto index = rect_to_image_index(width, height, rect);

			auto r_july = july[index * 4 + 0];
			auto g_july = july[index * 4 + 1];
			auto b_july = july[index * 4 + 2];

			auto r_jan = january[index * 4 + 0];
			auto g_jan = january[index * 4 + 1];
			auto b_jan = january[index * 4 + 2];

			auto flow_july = color_waterflow(r_july, g_july, b_july);
			auto flow_january = color_waterflow(r_jan, g_jan, b_jan);

			auto land =
				color_is_land(r_july, g_july, b_july)
				|| color_is_land(r_jan, g_jan, b_jan);

			auto is_fresh =
				color_is_fresh(r_july, g_july, b_july)
				|| color_is_land(r_jan, g_jan, b_jan);

			state.tile_set_is_land(tile, land);
			state.tile_set_is_fresh(tile, is_fresh);
			state.tile_set_july_waterflow(tile, flow_july);
			state.tile_set_january_waterflow(tile, flow_january);
			state.tile_set_waterlevel(tile, 0);

			if ((flow_july + flow_january) > 2000.0) {
				state.tile_set_has_river(tile, true);
			}


		});

		state.for_each_tile([&](dcon::tile_id tile) {
			auto is_land = state.tile_get_is_land(tile);
			for (int i = 0; i < 4; i++) {
				auto n = state.tile_get_neighbour(tile, i);
				auto n_is_land = state.tile_get_is_land(n);
				if (is_land != n_is_land) {
					state.tile_set_is_coast(tile, true);
					return;
				}
			}
		});
	}

	printf("Hydro maps are loaded\n");

	constexpr GLsizei cube_sides = 6;

	{
		printf("Loading heightmap...");

		std::string filename = "./lua/default/heightmap.png";
		uint8_t * img;
		int width, height, channels;

		img = stbi_load(
			filename.c_str(),
			&width,
			&height,
			&channels,
			4
		);

		state.for_each_tile([&](dcon::tile_id tile) {
			auto sphere = tile_to_sphere(world_size, tile);
			auto rect = sphere_to_rect(sphere);
			auto index = rect_to_image_index(width, height, rect);

			auto r = img[index * 4 + 0];
			auto g = img[index * 4 + 1];
			auto b = img[index * 4 + 2];

			auto sea_level = 94.f;
			auto elev = (float)r - sea_level;
			if (elev < 0) {
				elev = elev / sea_level * 8000.f;
			} else {
				elev = elev / (255.f - sea_level) * 8000.f;
			}
			state.tile_set_elevation(tile, elev);
		});
		printf("Heightmap loaded!\n");

		printf("Correcting elevation...");
		state.for_each_tile([&](dcon::tile_id tile) {
			auto elevation = state.tile_get_elevation(tile);
			if (state.tile_get_is_land(tile)) {
				state.tile_set_elevation(tile, std::max(1.f, elevation));
				state.tile_set_waterlevel(tile, 0);
			} else {
				state.tile_set_elevation(tile, std::min(-1.f, elevation));
				state.tile_set_waterlevel(tile, 0);
			}
		});
		printf("Elevation corrected!\n");
	}

	{
		printf("Loading soils...");

		std::string filename_depth = "./lua/default/soil-depth.png";
		std::string filename_organics = "./lua/default/soil-organics.png";
		std::string filename_minerals = "./lua/default/soil-minerals.png";
		std::string filename_texture = "./lua/default/soil-texture.png";
		uint8_t * depth;
		uint8_t * organics;
		uint8_t * minerals;
		uint8_t * texture;
		int width, height, channels;

		depth = stbi_load(
			filename_depth.c_str(),
			&width,
			&height,
			&channels,
			4
		);
		organics = stbi_load(
			filename_organics.c_str(),
			&width,
			&height,
			&channels,
			4
		);
		minerals = stbi_load(
			filename_minerals.c_str(),
			&width,
			&height,
			&channels,
			4
		);
		texture = stbi_load(
			filename_texture.c_str(),
			&width,
			&height,
			&channels,
			4
		);

		state.for_each_tile([&] (dcon::tile_id tile) {

			auto sphere = tile_to_sphere(world_size, tile);
			auto rect = sphere_to_rect(sphere);
			auto index = rect_to_image_index(width, height, rect);


			auto depth_r = depth[index * 4 + 0];
			auto depth_g = depth[index * 4 + 1];
			auto depth_b = depth[index * 4 + 2];

			auto organics_r = organics[index * 4 + 0];
			auto organics_g = organics[index * 4 + 1];
			auto organics_b = organics[index * 4 + 2];

			auto minerals_r = minerals[index * 4 + 0];
			auto minerals_g = minerals[index * 4 + 1];
			auto minerals_b = minerals[index * 4 + 2];

			auto texture_r = texture[index * 4 + 0];
			auto texture_g = texture[index * 4 + 1];
			auto texture_b = texture[index * 4 + 2];

			float total = (float)texture_r + (float)texture_g + (float)texture_b;
			if (total == 0) {
				total = 0.001f;
			}
			auto sand = texture_r / total;
			auto silt = texture_g / total;
			auto clay = texture_b / total;

			auto depth_hsv = rgb_to_hsv(
				(float) depth_r / 255.f,
				(float) depth_g / 255.f,
				(float) depth_b / 255.f
			);

			auto actual_depth = depth_hsv.x;
			auto depth = std::min(actual_depth, 235.f) / 235.f * 10.f;

			state.tile_set_sand(tile, sand * actual_depth);
			state.tile_set_silt(tile, silt * actual_depth);
			state.tile_set_clay(tile, clay * actual_depth);

			if (actual_depth == 0) {
				state.tile_set_soil_minerals(tile, 0);
				state.tile_set_soil_organics(tile, 0);
			} else {
				auto organics = rgb_to_hsv(organics_r, organics_g, organics_b).x;
				state.tile_set_soil_organics(tile, std::min(organics, 235.f) / 235.f);
				auto minerals = rgb_to_hsv(minerals_r, minerals_g, minerals_b).x;
				state.tile_set_soil_minerals(tile, std::min(minerals, 235.f) / 235.f);
			}
		});
		printf("Soils loaded!");
	}


	{
		printf("Loading ice...");

		std::string filename_ice = "./lua/default/ice.png";
		std::string filename_ice_age_ice = "./lua/default/ice-age-ice.png";
		uint8_t * ice;
		uint8_t * ice_age_ice;
		int width, height, channels;

		ice = stbi_load(
			filename_ice.c_str(),
			&width,
			&height,
			&channels,
			4
		);
		ice_age_ice = stbi_load(
			filename_ice_age_ice.c_str(),
			&width,
			&height,
			&channels,
			4
		);

		auto get_ice = [](uint8_t r, uint8_t g, uint8_t b) {
			if (g == 255 && b == 255) {
				if (r == 210)  return 10.f;
				else if (r == 225)  return 25.f;
				else if (r == 240)  return 40.f;
				else return 0.f;
			} else return 0.f;
		};

		state.for_each_tile([&](dcon::tile_id tile) {
			auto sphere = tile_to_sphere(world_size, tile);
			auto rect = sphere_to_rect(sphere);
			auto index = rect_to_image_index(width, height, rect);

			state.tile_set_ice(tile,
				get_ice(ice[index * 4], ice[index * 4 + 1], ice[index * 4 + 2])
			);
			state.tile_set_ice_age_ice(tile,
				get_ice(ice_age_ice[index * 4], ice_age_ice[index * 4 + 1], ice_age_ice[index * 4 + 2])
			);
		});
		printf("Ice loaded!");
	}

	{
		printf("Loading rocks");

		std::string rocks_filename = "./lua/default/rocks.png";
		uint8_t * rocks;
		int width, height, channels;

		rocks = stbi_load(
			rocks_filename.c_str(),
			&width,
			&height,
			&channels,
			4
		);

		// build a map for colors
		ankerl::unordered_dense::map<int32_t, dcon::bedrock_id> color_to_bedrock{};

		state.for_each_bedrock([&](auto bedrock){
			auto cid = rgb_to_id(
				state.bedrock_get_r(bedrock) * 255.f,
				state.bedrock_get_g(bedrock) * 255.f,
				state.bedrock_get_b(bedrock) * 255.f
			);
			color_to_bedrock[cid] = bedrock;
		});

		state.for_each_tile([&](dcon::tile_id tile) {
			auto sphere = tile_to_sphere(world_size, tile);
			auto rect = sphere_to_rect(sphere);
			auto index = rect_to_image_index(width, height, rect);

			auto cid = rgb_to_id(rocks[4 * index], rocks[4 * index + 1], rocks[4 * index + 2]);

			auto it = color_to_bedrock.find(cid);

			if (it == color_to_bedrock.end()) {
				state.tile_set_bedrock(tile, dcon::bedrock_id{7});
			} else {
				state.tile_set_bedrock(tile, it->second);
			}
		});
		printf("Rocks loaded!");
	}


	lua_getfield(L, LUA_GLOBALSINDEX, "sote");
	// [traceback, sote

	lua_getfield(L, -1, "load_world");
	// [traceback, sote, load_world

	result = lua_pcall(L, 0, 0, -3);
	// [traceback, sote

	if (result) exit(1);

	lua_pop(L, 1);
	// [traceback

	GLsizei map_mode_resolution = world_size;
	constexpr GLsizei map_mode_layers = 6;


	map_mode_data.resize(4 * map_mode_layers * map_mode_resolution * map_mode_resolution);

	glGenTextures(1, &map_mode_texture);
	glBindTexture(GL_TEXTURE_2D_ARRAY, map_mode_texture);
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	auto push_map_mode = [&]{
		glTexImage3D(
			GL_TEXTURE_2D_ARRAY,
			0,
			GL_RGBA,
			map_mode_resolution, map_mode_resolution, map_mode_layers,
			0,
			GL_RGBA, GL_UNSIGNED_BYTE, map_mode_data.data()
		);
	};

	generate_cube_sphere(state, world.map, world_size, false);
	generate_cube_sphere(state, world.sky, world_size, true);

	{
		// really basic
		// to be replaced with whatever squealing is doing in the shadows
		printf("Spawn races");
		state.pop_resize_dna(20);
		state.for_each_race([&](dcon::race_id race){
			state.for_each_tile([&](dcon::tile_id tile){
				auto settlement = state.tile_get_settlement_from_settlement_tile(tile);
				if (settlement) return;
				if (!state.tile_get_is_land(tile)) return;


				if (state.race_get_requires_large_river(race)) {
					if (!state.tile_get_has_river(tile)) return;
				}

				if (state.race_get_requires_large_forest(race)) {
					if(state.tile_get_conifer(tile) + state.tile_get_broadleaf(tile) < 0.5) return;
				}

				auto elevation = state.tile_get_elevation(tile);

				auto january_temp = state.tile_get_january_temperature(tile);
				auto july_temp = state.tile_get_july_temperature(tile);
				auto min_temp = std::min(january_temp, july_temp);
				auto avg_temp = (july_temp + january_temp) / 2.f;

				if (state.race_get_minimum_comfortable_temperature(race) > avg_temp) return;
				if (state.race_get_minimum_absolute_temperature(race) > min_temp) return;
				if (state.race_get_minimum_comfortable_elevation(race) > elevation) return;

				if (world.uniform(world.rng) > 0.0001f) return;

				auto s = state.create_settlement();
				state.force_create_settlement_tile(s, tile);

				auto leader = state.create_pop();
				state.force_create_pop_location(s, leader);
				state.pop_set_race(leader, race);
				// set dna
				for (int i = 0; i < 20; i ++) {
					state.pop_set_dna(leader, i, world.uniform(world.rng));
				}
			});
		});
	}

	lua_pop(L, 1);
	// [
}

void set_text(
	std::string text,
	dcon::data_container& state,
	text::font_manager& font_collection,
	text::layout& internal_layout,
	int size_x, int size_y,
	template_project::text_region_template region,
	dcon::locale_id current_locale,
	int grid_unit,
	bool is_header,
	float ui_scale
) {
	text::font_id font = state.locale_get_resolved_body_font(current_locale);
	if (is_header) {
		font = state.locale_get_resolved_header_font(current_locale);
	}

	auto native_rtl = state.locale_get_native_rtl(current_locale);
	auto rtl =
		native_rtl
		? text::layout_base::rtl_status::rtl
		: text::layout_base::rtl_status::ltr;

	internal_layout.contents.clear();
	internal_layout.number_of_lines = 0;

	text::single_line_layout sl{
		internal_layout,
		text::layout_parameters{
			0, 0,
			static_cast<int16_t>(size_x - region.h_text_margins * example_ui_project.grid_size * 2),
			static_cast<int16_t>(size_y - region.v_text_margins * 2),
			(uint16_t) (region.font_scale * grid_unit),
			font,
			0,
			alice_ui::convert_align(region.h_text_alignment),
			text::text_color::green,
			true,
			true
		},
		rtl
	};

	text::add_to_layout_box(
		state,
		font_collection,
		sl,
		sl.box,
		simple_fs::utf8_to_utf16(text),
		text::text_color::black,
		std::monostate{},
		state.locale_get_body_font_features(current_locale),
		(hb_script_t)state.locale_get_hb_script(current_locale),
		state.locale_get_resolved_language(current_locale),
		native_rtl,
		ui_scale
	);
}

void update_ui(
	lua_State* L,
	dcon::data_container& state,
	text::font_manager& font_collection,
	open_project_t& project,
	template_project::project& ui_templates,
	window_element_wrapper_t& window_prototype,
	window_element_data_container_t& window_instance,
	dcon::locale_id current_locale,
	float ui_scale
) {
	if (window_instance.children.size() == 0) {
		window_instance.children.resize(window_prototype.children.size());
	}

	lua_pushcfunction(L, traceback);
	// [traceback

	std::string project_name = simple_fs::native_to_utf8(project.project_name);

	lua_getfield(L, LUA_GLOBALSINDEX, "UI_LOGIC");
	if (lua_isnil(L, -1)) {
		window::emit_error_message("Missing " + project_name + "  UI_LOGIC table!", true);
	}
	// [traceback, UI_LOGIC

	lua_getfield(L, -1, project_name.c_str());
	// [traceback, UI_LOGIC, project name
	if (lua_isnil(L, -1)) {
		window::emit_error_message("Missing " + project_name + "  lua table!", true);
	}

	lua_getfield(L, -1, window_prototype.wrapped.name.c_str());
	if (lua_isnil(L, -1)) {
		window::emit_error_message(
			"Missing " + project_name + "." + window_prototype.wrapped.name + " lua table!",
			true
		);
	}
	// [traceback, UI_LOGIC, project name, window name

	int result;

	for(int item_index = 0; item_index < window_prototype.children.size(); item_index++) {
		auto& item_prototype = window_prototype.children[item_index];
		auto& item_instance = window_instance.children[item_index];

		std::string item_name = item_prototype.name;
		lua_getfield(L, -1, item_name.c_str());
		// [traceback, project name, window name, item name
		if (lua_isnil(L, -1)) {
			window::emit_error_message("Missing " + project_name + "." + item_name + " lua table!", true);
		}

		auto template_id = item_prototype.template_id;
		if (template_id == -1) {
			window::emit_error_message(
				"Element " + item_prototype.name + " from "+ project_name + " has no template!",
				true
			);
		}

		bool has_text = false;
		auto item_type = item_prototype.ttype;
		template_project::text_region_template region;
		if(item_type == template_project::template_type::label) {
			region = ui_templates.label_t[template_id].primary;
			has_text = true;
		} else if(item_type == template_project::template_type::button) {
			region = ui_templates.button_t[template_id].primary;
			has_text = true;
		}

		if (has_text) {
			lua_getfield(L, -1, "text");
			// [traceback, UI_LOGIC, project name, window name, item name, text_getter
			result = lua_pcall(L, 0, LUA_MULTRET, -6);
			if (result) exit(1);
			// [traceback, UI_LOGIC, project name, window name, item name, actual text
			std::string text = lua_tostring(L, -1);
			// [traceback, UI_LOGIC, project name, window name, item name, actual text

			set_text(
				text,
				state,
				font_collection,
				item_instance.internal_layout,
				item_prototype.x_size,
				item_prototype.y_size,
				region,
				current_locale,
				project.grid_size,
				item_prototype.text_type == text_type::header,
				ui_scale
			);

			lua_pop(L, 1);
			// [traceback, UI_LOGIC, project name, window name, item name
		}

		lua_pop(L, 1);
		// [traceback, UI_LOGIC, project name, window name
	}

	lua_pop(L, 4);
	// [
}

struct mouse_click {
	double x;
	double y;
	double time;
	bool release;
};

mouse_click clicks_buffer[256];
uint8_t clicks_buffer_left=0;
uint8_t clicks_buffer_right=0;

void handle_ui_click(
	lua_State* L,
	mouse_probe& probe,
	open_project_t& project,
	window_element_data_container_t& window,
	window_element_wrapper_t& window_prototype
){
	if (probe.control_id == -1) return;
	auto& c = window.children[probe.control_id];
	lua_pushcfunction(L, traceback);
	// [traceback

	std::string project_name = simple_fs::native_to_utf8(project.project_name);

	lua_getfield(L, LUA_GLOBALSINDEX, "UI_LOGIC");
	if (lua_isnil(L, -1)) {
		window::emit_error_message("Missing " + project_name + "  UI_LOGIC table!", true);
	}
	// [traceback, UI_LOGIC

	lua_getfield(L, -1, project_name.c_str());
	// [traceback, UI_LOGIC, project name
	if (lua_isnil(L, -1)) {
		window::emit_error_message("Missing " + project_name + "  lua table!", true);
	}

	lua_getfield(L, -1, window_prototype.wrapped.name.c_str());
	if (lua_isnil(L, -1)) {
		window::emit_error_message(
			"Missing " + project_name + "." + window_prototype.wrapped.name + " lua table!",
			true
		);
	}

	int result;
	auto& item_prototype = window_prototype.children[probe.control_id];
	auto& item_instance = window.children[probe.control_id];

	std::string item_name = item_prototype.name;
	lua_getfield(L, -1, item_name.c_str());
	// [traceback, project name, window name, item name
	if (lua_isnil(L, -1)) {
		window::emit_error_message("Missing " + project_name + "." + item_name + " lua table!", true);
	}

	auto template_id = item_prototype.template_id;
	if (template_id == -1) {
		window::emit_error_message(
			"Element " + item_prototype.name + " from "+ project_name + " has no template!",
			true
		);
	}

	bool active = false;
	auto item_type = item_prototype.ttype;
	template_project::text_region_template region;
	if(item_type == template_project::template_type::button) {
		region = ui_templates.button_t[template_id].primary;
		active = true;
	}

	if (active) {
		lua_getfield(L, -1, "left_click");
		// [traceback, UI_LOGIC, project name, window name, item name, click handler
		if (lua_isnil(L, -1)) {
			window::emit_error_message("Missing " + project_name + "." + window_prototype.wrapped.name + "." + item_name + ".left_click function!", true);
		}

		result = lua_pcall(L, 0, LUA_MULTRET, -6);
		if (result) exit(1);
		// [traceback, UI_LOGIC, project name, window name, item name
	}

	lua_pop(L, 5);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddMouseButtonEvent(button, action == GLFW_PRESS);

	// (2) ONLY forward mouse data to your underlying app/game.
	if (io.WantCaptureMouse) return;

	double mouse_x;
	double mouse_y;
	glfwGetCursorPos(window, &mouse_x, &mouse_y);

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		clicks_buffer[clicks_buffer_right].release = false;
		clicks_buffer[clicks_buffer_right].x = mouse_x;
		clicks_buffer[clicks_buffer_right].y = mouse_y;
		clicks_buffer_right++;
	} else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		clicks_buffer[clicks_buffer_right].release = true;
		clicks_buffer[clicks_buffer_right].x = mouse_x;
		clicks_buffer[clicks_buffer_right].y = mouse_y;
		clicks_buffer_right++;
	}
}

int main(void) {
	simple_fs::add_root(common_fs, NATIVE("./"));
	auto root = get_root(common_fs);
	auto assets = simple_fs::open_directory(root, NATIVE("assets"));

	auto example = simple_fs::open_file(assets, NATIVE("example_window.aui"));
	auto example_content = view_contents(*example);
	serialization::in_buffer example_buffer(example_content.data, example_content.file_size);
	example_ui_project = bytes_to_project(example_buffer);
	example_ui_project.project_name = L"example_window";
	example_ui_project.project_directory = NATIVE("./assets/");


	auto uitemplates = simple_fs::open_file(assets, NATIVE("the.tui"));
	auto content = view_contents(*uitemplates);
	serialization::in_buffer buffer(content.data, content.file_size);
	ui_templates = template_project::bytes_to_project(buffer);

	svg_image_files.root_directory = simple_fs::utf16_to_native(ui_templates.svg_directory);

	// ui_templates.project_name = rem.substr(0, ext_pos);
	// ui_templates.project_directory = example_ui_project.project_directory;
	for(auto& i : ui_templates.icons) {
		simple_fs::file loaded_file{  example_ui_project.project_directory + svg_image_files.root_directory + simple_fs::utf8_to_native(i.file_name) };
		i.renders = asvg::simple_svg(loaded_file.content.data, size_t(loaded_file.content.file_size));
	}
	for(auto& b : ui_templates.backgrounds) {
		simple_fs::file loaded_file{  example_ui_project.project_directory + svg_image_files.root_directory + simple_fs::utf8_to_native(b.file_name) };
		b.renders = asvg::svg(loaded_file.content.data, size_t(loaded_file.content.file_size), b.base_x, b.base_y);
	}

	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
		return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

	GLFWwindow* window;
	float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
	window = glfwCreateWindow(1280 * main_scale, 960 * main_scale, "009", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}
	glfwSetKeyCallback(window, key_callback);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	// ImGui::StyleColorsDark();
	ImGui::StyleColorsLight();

	geometry::world::point camera { {0.f, 0.f} };
	float zoom = 0.1f;

	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
	style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)


	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330 core");

	GLenum err = glewInit();
	if (GLEW_OK != err) {
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	}
	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	// GLEW validation
	if (auto result = glewInit(); result != GLEW_NO_ERROR)
		glew_fail("glewInit: ", result);
	if (!GLEW_VERSION_3_3)
		throw std::runtime_error("OpenGL 3.3 is not supported");

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);
	glEnable(GL_LINE_SMOOTH);

	// illumination settings
	glm::vec3 light_color = glm::vec3(3.f, 3.f, 3.f);
	glm::vec3 ambient_color = glm::vec3(0.2f, 0.2f, 0.4f);
	glClearColor(ambient_color.x, ambient_color.y, ambient_color.z, 0.f);

	// load alice ui shader

	glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);
	glEnable(GL_LINE_SMOOTH);

	ogl::data ogl_state;

	ogl::load_shaders(ogl_state, common_fs); // create shaders
	ogl::load_global_squares(ogl_state); // create various squares to drive the shaders with
	ogl::load_special_icons(ogl_state, common_fs);

	text::font_manager font_collection {};

	auto loc = simple_fs::open_directory(assets, NATIVE("localization"));
	for(auto& ld : simple_fs::list_subdirectories(loc)) {
		auto def_file = simple_fs::open_file(ld, NATIVE("locale.txt"));
		if(def_file) {
			auto contents = simple_fs::view_contents(*def_file);
			auto ld_name = simple_fs::get_full_name(ld);
			auto dir_lname = ld_name.substr(ld_name.find_last_of(NATIVE_DIR_SEPARATOR) + 1);
			printf("locale.txt discovered\n");
			locale::add_locale(state, simple_fs::native_to_utf8(dir_lname), contents.data, contents.data + contents.file_size);
		}
	}

	bool locale_loaded = false;
	dcon::locale_id current_locale {};
	for(auto l : state.in_locale) {
		auto ln = l.get_locale_name();
		auto ln_sv = std::string_view{ (char const*)ln.begin(), ln.size() };
		if(ln_sv == "en-US") {
			font_collection.resolve_locale(state, common_fs, l);
			locale_loaded = true;
			current_locale = l;
			break;
		}
	}
	assert(locale_loaded);

	static auto ink_color = template_project::color_by_name(ui_templates, "ink");

	load_shaders();
	load_global_squares();

	// setting up a basic shader
	std::string shader_2d_vertex_path = "./shaders/basic_shader_flat.vert";
	std::string shader_2d_fragment_path = "./shaders/basic_shader_flat.frag";
	std::string shader_2d_vertex_source = read_shader( shader_2d_vertex_path );
	std::string shader_2d_fragment_source = read_shader( shader_2d_fragment_path );
	auto shader_2d = create_program(
		create_shader(GL_VERTEX_SHADER, shader_2d_vertex_source.c_str()),
		create_shader(GL_FRAGMENT_SHADER, shader_2d_fragment_source.c_str())
	);
	shader_2d_data shader_2d_loc {};
	shader_2d_loc.shift = glGetUniformLocation(shader_2d, "shift");
	shader_2d_loc.zoom = glGetUniformLocation(shader_2d, "zoom");
	shader_2d_loc.aspect_ratio = glGetUniformLocation(shader_2d, "aspect_ratio");

	// setting up an extra basic shader

	std::string extra_basic_shader_vertex_path = "./shaders/basic_shader_textured.vert";
	std::string extra_basic_shader_fragment_path = "./shaders/basic_shader_textured.frag";

	std::string vertex_extra_basic_shader_source = read_shader( extra_basic_shader_vertex_path );
	std::string fragment_extra_basic_shader_source = read_shader( extra_basic_shader_fragment_path );

	auto extra_basic_shader = create_program(
		create_shader(GL_VERTEX_SHADER, vertex_extra_basic_shader_source.c_str()),
		create_shader(GL_FRAGMENT_SHADER, fragment_extra_basic_shader_source.c_str())
	);

	GLuint extra_basic_model_location = glGetUniformLocation(extra_basic_shader, "model");
	GLuint extra_basic_view_location = glGetUniformLocation(extra_basic_shader, "view");
	GLuint extra_basic_projection_location = glGetUniformLocation(extra_basic_shader, "projection");
	GLuint extra_basic_texture_location = glGetUniformLocation(extra_basic_shader, "texture_sampler");
	GLuint extra_basic_uv_mod_location = glGetUniformLocation(extra_basic_shader, "uv_mod");

	// setting up a basic shader
	std::string basic_shader_vertex_path = "./shaders/basic_shader_meshes.vert";
	std::string basic_shader_fragment_path = "./shaders/basic_shader_meshes.frag";

	std::string vertex_shader_source = read_shader( basic_shader_vertex_path );
	std::string fragment_shader_source = read_shader( basic_shader_fragment_path );

	auto basic_shader = create_program(
		create_shader(GL_VERTEX_SHADER, vertex_shader_source.c_str()),
		create_shader(GL_FRAGMENT_SHADER, fragment_shader_source.c_str())
	);

	GLuint model_location = glGetUniformLocation(basic_shader, "model");
	GLuint view_location = glGetUniformLocation(basic_shader, "view");
	GLuint projection_location = glGetUniformLocation(basic_shader, "projection");
	GLuint albedo_location = glGetUniformLocation(basic_shader, "albedo");
	GLuint map_data_location = glGetUniformLocation(basic_shader, "map_data");
	GLuint color_location = glGetUniformLocation(basic_shader, "color");
	GLuint use_texture_location = glGetUniformLocation(basic_shader, "use_texture");
	GLuint light_direction_location = glGetUniformLocation(basic_shader, "light_direction");
	GLuint camera_position_location = glGetUniformLocation(basic_shader, "camera_position");
	GLuint light_color_location = glGetUniformLocation(basic_shader, "light_color");
	GLuint ambient_location = glGetUniformLocation(basic_shader, "ambient");
	GLuint bones_location = glGetUniformLocation(basic_shader, "bones");

	GLuint shadow_layers_location = glGetUniformLocation(basic_shader, "shadow_layers");
	GLuint sky_flag_location = glGetUniformLocation(basic_shader, "sky_sphere");
	GLuint shadow_map_location = glGetUniformLocation(basic_shader, "shadow_map");
	GLuint render_shadow_transform_location = glGetUniformLocation(basic_shader, "shadow_transform");


	std::string shadow_vertex_path = "./shaders/shadow.vert";
	std::string shadow_fragment_path = "./shaders/shadow.frag";
	std::string shadow_vertex_shader_source = read_shader( shadow_vertex_path );
	std::string shadow_fragment_shader_source = read_shader( shadow_fragment_path );
	auto shadow_vertex_shader = create_shader(GL_VERTEX_SHADER, shadow_vertex_shader_source.c_str());
	auto shadow_fragment_shader = create_shader(GL_FRAGMENT_SHADER, shadow_fragment_shader_source.c_str());
	auto shadow_program = create_program(shadow_vertex_shader, shadow_fragment_shader);
	GLuint shadow_model_location = glGetUniformLocation(shadow_program, "model");
	GLuint shadow_transform_location = glGetUniformLocation(shadow_program, "transform");

	GLsizei shadow_map_resolution = 2048 * 2;
	const GLsizei shadow_layers = 1;

	GLuint shadow_map;
	glGenTextures(1, &shadow_map);
	glBindTexture(GL_TEXTURE_2D_ARRAY, shadow_map);
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage3D(
		GL_TEXTURE_2D_ARRAY,
		0,
		GL_RG32F,
		shadow_map_resolution, shadow_map_resolution, shadow_layers,
		0,
		GL_RGBA, GL_FLOAT, nullptr
	);

	std::vector<GLuint> shadow_fbo{};
	std::vector<GLuint> shadow_renderbuffers{};

	shadow_fbo.resize(shadow_layers);
	shadow_renderbuffers.resize(shadow_layers);

	for (GLsizei i = 0; i < shadow_layers; i ++) {
		glGenFramebuffers(1, &shadow_fbo[i]);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadow_fbo[i]);
		glFramebufferTextureLayer(
		GL_DRAW_FRAMEBUFFER,
		GL_COLOR_ATTACHMENT0,
		shadow_map, 0, i
		);

		if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		throw std::runtime_error("Incomplete framebuffer!");

		glGenRenderbuffers(1, &shadow_renderbuffers[i]);
		glBindRenderbuffer(GL_RENDERBUFFER, shadow_renderbuffers[i]);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, shadow_map_resolution, shadow_map_resolution);
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, shadow_renderbuffers[i]);
	}

	glm::vec3 albedo_world {0.4f, 0.5f, 0.8f};
	float albedo_character[] = {0.9f, 0.5f, 0.6f};
	float albedo_critter[] = {0.5f, 0.1f, 0.1f};


	std::default_random_engine rng;
	std::uniform_real_distribution<float> uniform{0.0, 1.0};
	std::normal_distribution<float> normal_d{0.f, 0.1f};
	std::normal_distribution<float> size_d{1.f, 0.3f};

	glm::vec3 camera_position{0.f, 0.f, 2.f};

	int width = 1;
	int height = 1;

	assert_no_errors();

	int status, result, i;
	lua_State *L;
	L = luaL_newstate();
	luaL_openlibs(L);

	status = luaL_loadfile(L, "./lua/main.lua");
	if (status) {
		fprintf(stderr, "Couldn't load file: %s\n", lua_tostring(L, -1));
		exit(1);
	}
	lua_newtable(L);
	lua_setglobal(L, "love");

	lua_newtable(L);
	lua_setglobal(L, "sote");

	lua_newtable(L);
	lua_setglobal(L, "UI_LOGIC");

	result = lua_pcall(L, 0, LUA_MULTRET, 0);
	if (result) {
		if (result == LUA_ERRRUN) {
			// traceback at this point
			const char *err = lua_tostring(L,-1);
			std::cerr << err << "\n";

			// pop the error object
			lua_pop(L,1);
			exit(1);
		}
		fprintf(stderr, "ERROR %d. Failed to run script: %s\n", result, lua_tostring(L, -1));
		exit(1);
	}

	std::string path_to_ui_script = "./ui_scripts/";
	path_to_ui_script += simple_fs::native_to_utf8(example_ui_project.project_name) + ".lua";
	status = luaL_dofile(L,  path_to_ui_script.c_str());
	if (status) {
		fprintf(stderr, "Couldn't load file: %s\n", lua_tostring(L, -1));
		exit(1);
	}

	auto& example_window = example_ui_project.windows[0];
	window_element_data_container_t example_window_instance {};

	settings current_settings {};
	current_settings.ui_scale = 1.f;

	update_ui(
		L, state, font_collection,
		example_ui_project, ui_templates,
		example_window, example_window_instance,
		current_locale, current_settings.ui_scale
	);

	game::simple_mesh square {};
	generate_square(square);

	std::vector<uint8_t> map_mode_data;

	GLuint map_mode_texture;
	int world_size = 1;

	float update_timer = 0.f;
	glm::vec3 light_direction {0.5f, 0.5f, 0.5f};
	glm::vec2 camera_speed = {};
	int tick = 0;
	float data[512] {};


	world_rendering_data world_opengl_data {
		.shadow_layers = shadow_layers,
		.shadow_fbo = shadow_fbo,
		.shadow_renderbuffers = shadow_renderbuffers,
		.shadow_map_resolution = shadow_map_resolution,
		.shadow_shader {
			.program = shadow_program,
			.model = shadow_model_location,
			.view = shadow_transform_location
		},
		.data_shader {
			.program = basic_shader,
			.model = model_location,
			.view = view_location,
			.projection = projection_location,
			.light_direction = light_direction_location,
			.light_color = light_color_location,
			.ambient_color = ambient_location,
			.albedo_color = albedo_location,
			.camera_position = camera_position_location,
			.map_data = map_data_location,
			.shadow_map = shadow_map_location,
			.shadow_layers = shadow_layers_location,
			.is_sky = sky_flag_location,
			.shadow_projection = shadow_transform_location,
		},
		.shadow_map_texture = shadow_map,
		.map_mode_texture = map_mode_texture,
		.albedo_world = albedo_world
	};

	std::string path_to_bg = "./lua/data/gfx/backgrounds/background.png";
	auto bg_key = new_text(state, game_text, path_to_bg.size(), path_to_bg.data());
	load_texture(game_text, bg_key);

	camera_data camera_opengl_data {};


	mouse_probe probe;

	glfwSetMouseButtonCallback(window, mouse_button_callback);

	double last_time = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{
		tick++;

		double time = glfwGetTime();
		float dt = (float)(time - last_time);
		last_time = time;

		update_timer += dt;

		if (update_timer > 1.f / 60.f) {
			update_timer = 0.f;
		}

		if (current_scene == game_scene::world_exploration) {
			update_camera(
				camera_opengl_data,
				world_size,
				dt,
				width,
				height
			);
		}

		// if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
		// 	ImGui_ImplGlfw_Sleep(10);
		// 	continue;
		// }

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

		// IMGUI CODE HERE
		if (settings_opened) {
			ImGui::Begin("Settings");

			float old_scale = current_settings.ui_scale;

			ImGui::SliderFloat(
				"UI scale", &current_settings.ui_scale, 0.25f, 4.f,
				"%.2f", ImGuiSliderFlags_AlwaysClamp
			);

			current_settings.ui_scale = round(current_settings.ui_scale * 4.f) / 4.f;

			if (old_scale != current_settings.ui_scale) {
				update_ui(
					L, state, font_collection,
					example_ui_project, ui_templates,
					example_window, example_window_instance,
					current_locale, current_settings.ui_scale
				);
			}

			ImGui::End();
		}

		ImGui::Render();

		double mouse_x;
		double mouse_y;
		glfwGetCursorPos(window, &mouse_x, &mouse_y);

		probe.x = mouse_x;
		probe.y = mouse_y;
		probe.last_frame_control_id = probe.control_id;
		probe.last_frame_window_id = probe.window_id;
		if (io.WantCaptureMouse) {
			probe.last_frame_control_id = -1;
			probe.last_frame_window_id = -1;
		}
		probe.control_id = -1;
		probe.window_id = -1;


		// OPENGL RENDERING HERE

		if (current_scene == game_scene::main_menu) {
			handle_main_menu(
				ogl_state, font_collection,
				example_window, example_window_instance,
				width, height, probe, bg_key, current_settings.ui_scale
			);
		} else if (current_scene == game_scene::world_exploration) {
			render_world(
				window,
				world_opengl_data,
				camera_opengl_data,
				map_mode_data,
				light_direction,
				ambient_color,
				light_color,
				width,
				height
			);
		}

		// conclusion

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glfwGetFramebufferSize(window, &width, &height);
		width = std::max(width, 10);
		height = std::max(height, 10);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
		assert_no_errors();

		while (clicks_buffer_left != clicks_buffer_right) {
			if (clicks_buffer[clicks_buffer_left].release) {
				handle_ui_click(
					L,
					probe,
					example_ui_project,
					example_window_instance,
					example_window
				);
			}
			clicks_buffer_left++;
		}
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(window);
	glfwTerminate();
	/* Cya, Lua */
	lua_close(L);
	return 0;
}