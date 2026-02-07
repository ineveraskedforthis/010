#include "opengl_wrapper.hpp"
#include "project_description.hpp"
#include "BlankProject/src/gamestate/uitemplate.hpp"
#include "asvg.hpp"
#include "simple_fs.hpp"
#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif
#include "GL/glew.h"

void load_global_squares();
void load_shaders();
void render_textured_rect(color3f color, float ix, float iy, int32_t iwidth, int32_t iheight, GLuint texture_handle);
void render_stretch_textured_rect(color3f color, float ix, float iy, float ui_scale, int32_t iwidth, int32_t iheight, float border_size, GLuint texture_handle);
void render_empty_rect(color3f color, float ix, float iy, int32_t iwidth, int32_t iheight);
void render_hollow_rect(color3f color, float ix, float iy, int32_t iwidth, int32_t iheight);
void render_layout_rect(color3f outline_color, float ix, float iy, int32_t iwidth, int32_t iheight);
void render_layout(
	ogl::data& ogl_state,
	simple_fs::file_system const& fs,
	text::font_manager& font_collection,
	asvg::file_bank& svg_image_files,
	open_project_t& open_project,
	template_project::project& open_templates,
	window_element_wrapper_t& window,
	window_element_data_container_t& window_instance,
	layout_level_t& layout,
	int layer,
	float x, float y,
	int32_t width, int32_t height,
	int32_t width_window, int32_t height_window,
	color3f outline_color, float scale
);
void render_window(
	ogl::data& ogl_state,
	simple_fs::file_system const& fs,
	text::font_manager& font_collection,
	asvg::file_bank& svg_image_files,
	open_project_t& open_project,
	template_project::project& open_templates,
	window_element_wrapper_t& win,
	window_element_data_container_t& win_instance,
	float x, float y,
	int width_game_window, int height_game_window,
	bool highlightwin,
	float ui_scale
);
void use_program(int display_w, int display_h);
