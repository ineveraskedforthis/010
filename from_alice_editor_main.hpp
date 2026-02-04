#include "AliceUIEditor/project_description.hpp"
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
	simple_fs::file_system const& fs,
	asvg::file_bank& svg_image_files,
        open_project_t& open_project,
        template_project::project& open_templates,
        window_element_wrapper_t& window,
        layout_level_t& layout, int layer, float x, float y, int32_t width, int32_t height, color3f outline_color, float scale
);
void render_window(
        simple_fs::file_system const& fs,
        asvg::file_bank& svg_image_files,
        open_project_t& open_project,
        template_project::project& open_templates,
        window_element_wrapper_t& win,
        float x,
        float y,
        bool highlightwin,
        float ui_scale
);
void use_program(int display_w, int display_h);
