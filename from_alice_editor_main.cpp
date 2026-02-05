// file is derived from alice UI editor

#include <variant>
#include "from_alice_editor_main.hpp"

std::string_view opengl_get_error_name(GLenum t);
std::string to_string(std::string_view str);
void assert_no_errors();

void update_cached_control(open_project_t& open_project, std::string_view name, window_element_wrapper_t& window, int16_t& index) {
	bool update = false;
	if(index < 0 || int16_t(window.children.size()) <= index)
		update = true;
	if(!update && window.children[index].name != name)
		update = true;
	if(update) {
		for(auto i = window.children.size(); i-- > 0; ) {
			if(window.children[i].name == name) {
				index = int16_t(i);
				return;
			}
		}
		index = int16_t(-1);
	}
}
void update_cached_window(open_project_t& open_project, std::string_view name, int16_t& index) {
	bool update = false;
	if(index < 0 || int16_t(open_project.windows.size()) <= index)
		update = true;
	if(!update && open_project.windows[index].wrapped.name != name)
		update = true;
	if(update) {
		for(auto i = open_project.windows.size(); i-- > 0; ) {
			if(open_project.windows[i].wrapped.name == name) {
				index = int16_t(i);
				return;
			}
		}
		index = int16_t(-1);
	}
}

struct measure_result {
	int32_t x_space;
	int32_t y_space;
	enum class special {
		none, space_consumer, end_line, end_page, no_break
	} other;
};

struct index_result {
	layout_item* result = nullptr;
	int32_t sub_index = 0;
};

struct layout_item_position {
	int32_t index = 0;
	int32_t sub_index = 0;

	bool operator==(layout_item_position const& o) const noexcept {
		return index == o.index && sub_index == o.sub_index;
	}
	bool operator!=(layout_item_position const& o) const noexcept {
		return !(*this == o);
	}
	bool operator<=(layout_item_position const& o) const noexcept {
		return (index < o.index) || (index == o.index && sub_index <= o.sub_index);
	}
	bool operator>=(layout_item_position const& o) const noexcept {
		return (index > o.index) || (index == o.index && sub_index >= o.sub_index);
	}
	bool operator<(layout_item_position const& o) const noexcept {
		return !(*this >= o);
	}
	bool operator>(layout_item_position const& o) const noexcept {
		return !(*this <= o);
	}
};

void render_control(
        simple_fs::file_system const& fs,
        asvg::file_bank& svg_image_files,
        open_project_t& open_project,
        template_project::project& open_templates,
        ui_element_t& c, float x, float y, float ui_scale
) {
	auto render_asvg_rect = [&](asvg::svg& s, float hcursor, float vcursor, float x_sz, float y_sz, int32_t gsz) {
		render_textured_rect(color3f{ 0.f, 0.f, 0.f },
			hcursor,
			vcursor,
			std::max(1, int32_t(x_sz * ui_scale)),
			std::max(1, int32_t(y_sz * ui_scale)),
			s.get_render(fs, svg_image_files, x_sz / float(gsz), y_sz / float(gsz), gsz, 2.0f));
	};
	auto render_svg_rect = [&](asvg::simple_svg& s, float hcursor, float vcursor, int32_t x_sz, int32_t y_sz, color3f c) {
		render_textured_rect(color3f{ 0.f, 0.f, 0.f },
			hcursor,
			vcursor,
			std::max(1, int32_t(x_sz * ui_scale)),
			std::max(1, int32_t(y_sz * ui_scale)),
			s.get_render(fs, svg_image_files, x_sz, y_sz, 2.0f, c.r, c.g, c.b));
	};

	if(c.ttype == template_project::template_type::label) {
		if(c.template_id != -1) {
			auto bg = open_templates.label_t[c.template_id].primary.bg;
			if(bg != -1)
				render_asvg_rect(open_templates.backgrounds[bg].renders, (x * ui_scale), (y * ui_scale),  c.x_size, c.y_size, open_project.grid_size);
			else
				render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
		} else {
			render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
		}
		return;
	}
	if(c.ttype == template_project::template_type::button) {
		if(c.template_id != -1) {
			auto bg = open_templates.button_t[c.template_id].primary.bg;
			if(bg != -1)
				render_asvg_rect(open_templates.backgrounds[bg].renders, (x * ui_scale), (y * ui_scale), c.x_size, c.y_size, open_project.grid_size);
			else
				render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
		} else {
			render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
		}
		return;
	}
	if(c.ttype == template_project::template_type::legacy_control) {
		render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
		return;
	}
	if(c.ttype == template_project::template_type::edit_control) {
		if(c.template_id != -1) {
			auto bg = open_templates.button_t[c.template_id].primary.bg;
			if(bg != -1)
				render_asvg_rect(open_templates.backgrounds[bg].renders, (x * ui_scale), (y * ui_scale), c.x_size, c.y_size, open_project.grid_size);
			else
				render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
		} else {
			render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
		}
		return;
	}
	if(c.ttype == template_project::template_type::free_background) {
		if(c.template_id != -1)
			render_asvg_rect(open_templates.backgrounds[c.template_id].renders, (x * ui_scale), (y * ui_scale), c.x_size, c.y_size, open_project.grid_size);
		else
			render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
		return;
	}
	if(c.ttype == template_project::template_type::drop_down_control) {
		if(c.template_id != -1) {
			auto bg = open_templates.drop_down_t[c.template_id].primary_bg;
			if(bg != -1)
				render_asvg_rect(open_templates.backgrounds[bg].renders, (x * ui_scale), (y * ui_scale), c.x_size, c.y_size, open_project.grid_size);
			else
				render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
		} else {
			render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
		}
		return;
	}
	if(c.ttype == template_project::template_type::free_icon) {
		if(c.template_id != -1) {

			auto vcursor = y * ui_scale;
			auto hcursor = x * ui_scale;

			render_svg_rect(open_templates.icons[c.template_id].renders,
				hcursor, vcursor, int32_t((c.x_size)), int32_t((c.y_size)),
				c.table_divider_color);
		} else {
			render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
		}
		return;
	}
	if(c.ttype == template_project::template_type::drag_and_drop_target) {
		render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
		return;
	}
	if(c.ttype == template_project::template_type::stacked_bar_chart) {
		if(c.template_id != -1) {
			auto bg = open_templates.stacked_bar_t[c.template_id].overlay_bg;
			if(bg != -1)
				render_asvg_rect(open_templates.backgrounds[bg].renders, (x * ui_scale), (y * ui_scale), c.x_size, c.y_size, open_project.grid_size);
			else
				render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
		} else {
			render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
		}
		return;
	}
	if(c.ttype == template_project::template_type::iconic_button) {
		if(c.template_id != -1) {
			auto bg = open_templates.iconic_button_t[c.template_id].primary.bg;
			if(bg != -1)
				render_asvg_rect(open_templates.backgrounds[bg].renders, (x * ui_scale), (y * ui_scale), c.x_size, c.y_size, open_project.grid_size);
			else
				render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));

			auto vcursor = y * ui_scale;
			auto hcursor = x * ui_scale;
			if(c.icon_id != -1) {
				auto l = open_templates.iconic_button_t[c.template_id].primary.icon_left.resolve(float(c.x_size), float(c.y_size), open_project.grid_size) * ui_scale + hcursor;
				auto t = open_templates.iconic_button_t[c.template_id].primary.icon_top.resolve(float(c.x_size), float(c.y_size), open_project.grid_size) * ui_scale + vcursor;
				auto r = open_templates.iconic_button_t[c.template_id].primary.icon_right.resolve(float(c.x_size), float(c.y_size), open_project.grid_size) * ui_scale + hcursor;
				auto b = open_templates.iconic_button_t[c.template_id].primary.icon_bottom.resolve(float(c.x_size), float(c.y_size), open_project.grid_size) * ui_scale + vcursor;

				hcursor = l;
				vcursor = t;

                                auto color = open_templates.colors[open_templates.iconic_button_t[c.template_id].primary.icon_color];

				render_svg_rect(
                                        open_templates.icons[c.icon_id].renders,
					hcursor, vcursor,
                                        int32_t((r - l) / ui_scale), int32_t((b - t) / ui_scale),
                                        {color.r, color.g, color.b}
                                );
			}
		} else {
			render_empty_rect(c.rectangle_color, (x* ui_scale), (y* ui_scale), std::max(1, int32_t(c.x_size* ui_scale)), std::max(1, int32_t(c.y_size* ui_scale)));
		}
		return;
	}
	if(c.ttype == template_project::template_type::mixed_button) {
		if(c.template_id != -1) {
			auto bg = open_templates.mixed_button_t[c.template_id].primary.bg;
			if(bg != -1)
				render_asvg_rect(open_templates.backgrounds[bg].renders, (x * ui_scale), (y * ui_scale), c.x_size, c.y_size, open_project.grid_size);
			else
				render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));

			auto vcursor = y * ui_scale;
			auto hcursor = x * ui_scale;
			if(c.icon_id != -1) {
				auto l = open_templates.mixed_button_t[c.template_id].primary.icon_left.resolve(float(c.x_size), float(c.y_size), open_project.grid_size) * ui_scale + hcursor;
				auto t = open_templates.mixed_button_t[c.template_id].primary.icon_top.resolve(float(c.x_size), float(c.y_size), open_project.grid_size) * ui_scale + vcursor;
				auto r = open_templates.mixed_button_t[c.template_id].primary.icon_right.resolve(float(c.x_size), float(c.y_size), open_project.grid_size) * ui_scale + hcursor;
				auto b = open_templates.mixed_button_t[c.template_id].primary.icon_bottom.resolve(float(c.x_size), float(c.y_size), open_project.grid_size) * ui_scale + vcursor;

				hcursor = l;
				vcursor = t;

                                auto color = open_templates.colors[open_templates.mixed_button_t[c.template_id].primary.shared_color];

				render_svg_rect(open_templates.icons[c.icon_id].renders,
					hcursor, vcursor, int32_t((r - l) / ui_scale), int32_t((b - t) / ui_scale),
                                        {color.r, color.g, color.b}
				);

			}
		} else {
			render_empty_rect(c.rectangle_color, (x* ui_scale), (y* ui_scale), std::max(1, int32_t(c.x_size* ui_scale)), std::max(1, int32_t(c.y_size* ui_scale)));
		}
		return;
	}
	if(c.ttype == template_project::template_type::mixed_button_ci) {
		if(c.template_id != -1) {
			auto bg = open_templates.mixed_button_t[c.template_id].primary.bg;
			if(bg != -1)
				render_asvg_rect(open_templates.backgrounds[bg].renders, (x * ui_scale), (y * ui_scale), c.x_size, c.y_size, open_project.grid_size);
			else
				render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));

			auto vcursor = y * ui_scale;
			auto hcursor = x * ui_scale;
			if(c.icon_id != -1) {
				auto l = open_templates.mixed_button_t[c.template_id].primary.icon_left.resolve(float(c.x_size), float(c.y_size), open_project.grid_size) * ui_scale + hcursor;
				auto t = open_templates.mixed_button_t[c.template_id].primary.icon_top.resolve(float(c.x_size), float(c.y_size), open_project.grid_size) * ui_scale + vcursor;
				auto r = open_templates.mixed_button_t[c.template_id].primary.icon_right.resolve(float(c.x_size), float(c.y_size), open_project.grid_size) * ui_scale + hcursor;
				auto b = open_templates.mixed_button_t[c.template_id].primary.icon_bottom.resolve(float(c.x_size), float(c.y_size), open_project.grid_size) * ui_scale + vcursor;

				hcursor = l;
				vcursor = t;

				render_svg_rect(open_templates.icons[c.icon_id].renders,
					hcursor, vcursor, int32_t((r - l) / ui_scale), int32_t((b - t) / ui_scale),
					c.table_divider_color);

			}
		} else {
			render_empty_rect(c.rectangle_color, (x* ui_scale), (y* ui_scale), std::max(1, int32_t(c.x_size* ui_scale)), std::max(1, int32_t(c.y_size* ui_scale)));
		}
		return;
	}
	if(c.ttype == template_project::template_type::iconic_button_ci) {
		if(c.template_id != -1) {
			auto bg = open_templates.iconic_button_t[c.template_id].primary.bg;
			if(bg != -1)
				render_asvg_rect(open_templates.backgrounds[bg].renders, (x * ui_scale), (y * ui_scale), c.x_size, c.y_size, open_project.grid_size);
			else
				render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));

			auto vcursor = y * ui_scale;
			auto hcursor = x * ui_scale;
			if(c.icon_id != -1) {
				auto l = open_templates.iconic_button_t[c.template_id].primary.icon_left.resolve(float(c.x_size), float(c.y_size), open_project.grid_size) * ui_scale + hcursor;
				auto t = open_templates.iconic_button_t[c.template_id].primary.icon_top.resolve(float(c.x_size), float(c.y_size), open_project.grid_size) * ui_scale + vcursor;
				auto r = open_templates.iconic_button_t[c.template_id].primary.icon_right.resolve(float(c.x_size), float(c.y_size), open_project.grid_size) * ui_scale + hcursor;
				auto b = open_templates.iconic_button_t[c.template_id].primary.icon_bottom.resolve(float(c.x_size), float(c.y_size), open_project.grid_size) * ui_scale + vcursor;

				hcursor = l;
				vcursor = t;

				render_svg_rect(open_templates.icons[c.icon_id].renders,
					hcursor, vcursor, int32_t((r - l) / ui_scale), int32_t((b - t) / ui_scale),
					c.table_divider_color);

			}
		} else {
			render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
		}
		return;
	}
	if(c.ttype == template_project::template_type::toggle_button) {
		if(c.template_id != -1) {
			auto bg = open_templates.toggle_button_t[c.template_id].on_region.primary.bg;
			if(bg != -1)
				render_asvg_rect(open_templates.backgrounds[bg].renders, (x * ui_scale), (y * ui_scale), c.x_size, c.y_size, open_project.grid_size);
			else
				render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
		} else {
			render_empty_rect(c.rectangle_color, (x* ui_scale), (y* ui_scale), std::max(1, int32_t(c.x_size* ui_scale)), std::max(1, int32_t(c.y_size* ui_scale)));
		}
		return;
	}
	if(c.ttype == template_project::template_type::table_header || c.ttype == template_project::template_type::table_row || c.ttype == template_project::template_type::table_highlights) {
		auto t = table_from_name(open_project, c.table_connection);
		if(t) {
			if(!t->table_columns.empty()) {
				int16_t sum = 0;
				for(auto& col : t->table_columns) {
					render_empty_rect(c.rectangle_color, ((x + sum) * ui_scale), (y * ui_scale), std::max(1, int32_t(col.display_data.width * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
					sum += col.display_data.width;
				}
			}
		} else {
			render_empty_rect(c.rectangle_color, (x* ui_scale), (y* ui_scale), std::max(1, int32_t(c.x_size* ui_scale)), std::max(1, int32_t(c.y_size* ui_scale)));
		}
		return;
	}
	if(c.background == background_type::table_columns || c.background == background_type::table_headers) {
		auto t = table_from_name(open_project, c.table_connection);
		if(t) {
			if(!t->table_columns.empty()) {
				int16_t sum = 0;
				for(auto& col : t->table_columns) {
					render_empty_rect(c.rectangle_color, ((x + sum) * ui_scale), (y * ui_scale), std::max(1, int32_t(col.display_data.width * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
					sum += col.display_data.width;
				}
			}
		} else {
			render_empty_rect(c.rectangle_color, (x* ui_scale), (y* ui_scale), std::max(1, int32_t(c.x_size* ui_scale)), std::max(1, int32_t(c.y_size* ui_scale)));
		}
	} else if(c.background != background_type::texture && c.background != background_type::bordered_texture && c.background != background_type::progress_bar) {
		render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
	} else if(c.background == background_type::texture || c.background == background_type::progress_bar) {
		if(c.ogl_texture.loaded == false) {
			c.ogl_texture.load(open_project.project_directory + simple_fs::utf8_to_native(c.texture));
		}
		if(c.ogl_texture.texture_handle == 0) {
			render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
		} else {
			render_textured_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)), c.ogl_texture.texture_handle);
		}
	} else if(c.background == background_type::bordered_texture) {
		if(c.ogl_texture.loaded == false) {
			c.ogl_texture.load(open_project.project_directory + simple_fs::utf8_to_native(c.texture));
		}
		if(c.ogl_texture.texture_handle == 0) {
			render_empty_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)));
		} else {
			render_stretch_textured_rect(c.rectangle_color, (x * ui_scale), (y * ui_scale), ui_scale, std::max(1, int32_t(c.x_size * ui_scale)), std::max(1, int32_t(c.y_size * ui_scale)), c.border_size, c.ogl_texture.texture_handle);
		}
	}
}


void render_window(
        simple_fs::file_system const& fs,
        asvg::file_bank& svg_image_files,
        open_project_t& open_project,
        template_project::project& open_templates, window_element_wrapper_t& win, float x, float y, bool highlightwin, float ui_scale
) {
	// bg
	if(win.wrapped.template_id != -1) {
		auto render_asvg_rect = [&](asvg::svg& s, float hcursor, float vcursor, float x_sz, float y_sz, int32_t gsz) {
			render_hollow_rect(win.wrapped.rectangle_color * (highlightwin ? 1.0f : 0.8f),
				hcursor,
				vcursor,
				std::max(1, int32_t(x_sz * ui_scale)),
				std::max(1, int32_t(y_sz * ui_scale)));

			render_textured_rect(color3f{ 0.f, 0.f, 0.f },
				hcursor,
				vcursor,
				std::max(1, int32_t(x_sz * ui_scale)),
				std::max(1, int32_t(y_sz * ui_scale)),
				s.get_render(fs, svg_image_files, x_sz / float(gsz), y_sz / float(gsz), gsz, 2.0f));

		};
		auto render_svg_rect = [&](asvg::simple_svg& s, float hcursor, float vcursor, int32_t x_sz, int32_t y_sz, color3f c) {
			render_textured_rect(color3f{ 0.f, 0.f, 0.f },
				hcursor,
				vcursor,
				std::max(1, int32_t(x_sz * ui_scale)),
				std::max(1, int32_t(y_sz * ui_scale)),
				s.get_render(fs, svg_image_files, x_sz, y_sz, 2.0f, c.r, c.g, c.b));
		};

		auto& thm = open_templates;
		auto selected_template = win.wrapped.template_id;

		if(thm.window_t[selected_template].bg != -1) {
			render_asvg_rect(thm.backgrounds[thm.window_t[selected_template].bg].renders, x * ui_scale, y * ui_scale, win.wrapped.x_size, win.wrapped.y_size, open_project.grid_size);
		} else {
			render_empty_rect(win.wrapped.rectangle_color * (highlightwin ? 1.0f : 0.8f), (x * ui_scale), (y * ui_scale), std::max(1, int32_t(win.wrapped.x_size * ui_scale)), std::max(1, int32_t(win.wrapped.y_size * ui_scale)));
		}
		auto vcursor = (y + thm.window_t[selected_template].v_close_button_margin * open_project.grid_size) * ui_scale;
		auto hcursor = (x+ win.wrapped.x_size - thm.window_t[selected_template].h_close_button_margin * open_project.grid_size - open_project.grid_size * 3.0f) * ui_scale;
		if(win.wrapped.auto_close_button && thm.window_t[selected_template].close_button_definition != -1) {
			auto l = thm.iconic_button_t[thm.window_t[selected_template].close_button_definition].primary.icon_left.resolve(float(3 * open_project.grid_size), float(3 * open_project.grid_size), open_project.grid_size) * ui_scale + hcursor;
			auto t = thm.iconic_button_t[thm.window_t[selected_template].close_button_definition].primary.icon_top.resolve(float(3 * open_project.grid_size), float(3 * open_project.grid_size), open_project.grid_size) * ui_scale + vcursor;
			auto r = thm.iconic_button_t[thm.window_t[selected_template].close_button_definition].primary.icon_right.resolve(float(3 * open_project.grid_size), float(3 * open_project.grid_size), open_project.grid_size) * ui_scale + hcursor;
			auto b = thm.iconic_button_t[thm.window_t[selected_template].close_button_definition].primary.icon_bottom.resolve(float(3 * open_project.grid_size), float(3 * open_project.grid_size), open_project.grid_size) * ui_scale + vcursor;

			if(thm.iconic_button_t[thm.window_t[selected_template].close_button_definition].primary.bg != -1)
				render_asvg_rect(thm.backgrounds[thm.iconic_button_t[thm.window_t[selected_template].close_button_definition].primary.bg].renders, hcursor, vcursor, 3 * open_project.grid_size, 3 * open_project.grid_size, open_project.grid_size);

			hcursor = l;
			vcursor = t;

			if(thm.window_t[selected_template].close_button_icon != -1) {
                                auto color = thm.colors[thm.iconic_button_t[thm.window_t[selected_template].close_button_definition].primary.icon_color];
				render_svg_rect(thm.icons[thm.window_t[selected_template].close_button_icon].renders,
					hcursor, vcursor, int32_t((r - l) / ui_scale), int32_t((b - t) / ui_scale),
					{color.r, color.g, color.b}
                                );
			}
		}
	} else {
		if(
			win.wrapped.background == background_type::none
			|| win.wrapped.background == background_type::existing_gfx
			|| win.wrapped.texture.empty()
			|| win.wrapped.background == background_type::linechart
			|| win.wrapped.background == background_type::stackedbarchart
			|| win.wrapped.background == background_type::doughnut
			|| win.wrapped.background == background_type::colorsquare
			|| win.wrapped.background == background_type::border_texture_repeat
			|| win.wrapped.background == background_type::textured_corners
			) {
			render_empty_rect(win.wrapped.rectangle_color * (highlightwin ? 1.0f : 0.8f), (x * ui_scale), (y * ui_scale), std::max(1, int32_t(win.wrapped.x_size * ui_scale)), std::max(1, int32_t(win.wrapped.y_size * ui_scale)));
		} else if(win.wrapped.background == background_type::texture) {
			if(win.wrapped.ogl_texture.loaded == false) {
				win.wrapped.ogl_texture.load(open_project.project_directory + simple_fs::utf8_to_native(win.wrapped.texture));
			}
			if(win.wrapped.ogl_texture.texture_handle == 0) {
				render_empty_rect(win.wrapped.rectangle_color * (highlightwin ? 1.0f : 0.8f), (x * ui_scale), (y * ui_scale), std::max(1, int32_t(win.wrapped.x_size * ui_scale)), std::max(1, int32_t(win.wrapped.y_size * ui_scale)));
			} else {
				render_textured_rect(win.wrapped.rectangle_color * (highlightwin ? 1.0f : 0.8f), (x * ui_scale), (y * ui_scale), std::max(1, int32_t(win.wrapped.x_size * ui_scale)), std::max(1, int32_t(win.wrapped.y_size * ui_scale)), win.wrapped.ogl_texture.texture_handle);
			}
		} else if(win.wrapped.background == background_type::bordered_texture) {
			if(win.wrapped.ogl_texture.loaded == false) {
				win.wrapped.ogl_texture.load(open_project.project_directory + simple_fs::utf8_to_native(win.wrapped.texture));
			}
			if(win.wrapped.ogl_texture.texture_handle == 0) {
				render_empty_rect(win.wrapped.rectangle_color * (highlightwin ? 1.0f : 0.8f), (x * ui_scale), (y * ui_scale), std::max(1, int32_t(win.wrapped.x_size * ui_scale)), std::max(1, int32_t(win.wrapped.y_size * ui_scale)));
			} else {
				render_stretch_textured_rect(win.wrapped.rectangle_color * (highlightwin ? 1.0f : 0.8f), (x * ui_scale), (y * ui_scale), ui_scale, std::max(1, int32_t(win.wrapped.x_size * ui_scale)), std::max(1, int32_t(win.wrapped.y_size * ui_scale)), win.wrapped.border_size, win.wrapped.ogl_texture.texture_handle);
			}
		}
	}

	if(win.wrapped.share_table_highlight) {
		auto t = table_from_name(open_project, win.wrapped.table_connection);
		if(t) {
			if(!t->table_columns.empty()) {
				int16_t sum = 0;
				for(auto& col : t->table_columns) {
					render_empty_rect(win.wrapped.rectangle_color * (highlightwin ? 1.0f : 0.8f), ((x + sum) * ui_scale), (y * ui_scale), std::max(1, int32_t(col.display_data.width * ui_scale)), std::max(1, int32_t(win.wrapped.y_size * ui_scale)));
					sum += col.display_data.width;
				}
			}
		}
	}

	// layout
	render_layout(fs, svg_image_files, open_project, open_templates, win, win.layout, 1, x, y, win.wrapped.x_size, win.wrapped.y_size, win.wrapped.rectangle_color, ui_scale);
}

struct layout_iterator {
	std::vector<layout_item>& backing;
	layout_item_position position;

	layout_iterator(std::vector<layout_item>& backing) : backing(backing) {
	}

	bool current_is_glue() {
		return has_more() && std::holds_alternative<layout_glue_t>(backing[position.index]);
	}
	measure_result measure_current(open_project_t& open_project,  template_project::project& open_templates, window_element_wrapper_t& window, bool glue_horizontal, int32_t max_crosswise, bool first_in_section) {
		if(!has_more())
			return measure_result{ 0, 0, measure_result::special::none };
		auto& m = backing[position.index];

		if(std::holds_alternative<layout_control_t>(m)) {
			auto& i = std::get<layout_control_t>(m);
			update_cached_control(open_project, i.name, window, i.cached_index);

			if(i.absolute_position) {
				return  measure_result{ 0, 0, measure_result::special::none };
			}
			if(i.cached_index != -1) {
				measure_result res;
				res.other = measure_result::special::none;
				res.x_space = window.children[i.cached_index].x_size;
				res.y_space = window.children[i.cached_index].y_size;
				if(i.fill_x) {
					if(glue_horizontal) {
						res.other = measure_result::special::space_consumer;
						res.x_space = 0;
					} else {
						res.x_space = int16_t(max_crosswise);
					}
				}
				if(i.fill_y) {
					if(!glue_horizontal) {
						res.other = measure_result::special::space_consumer;
						res.y_space = 0;
					} else {
						res.y_space = int16_t(max_crosswise);
					}
				}
				return res;
			}
		} else if(std::holds_alternative<layout_window_t>(m)) {
			auto& i = std::get<layout_window_t>(m);
			update_cached_window(open_project, i.name, i.cached_index);
			if(i.absolute_position) {
				return  measure_result{ 0, 0, measure_result::special::none };
			}
			if(i.cached_index != -1) {
				measure_result res;
				res.other = measure_result::special::none;
				res.x_space = open_project.windows[i.cached_index].wrapped.x_size;
				res.y_space = open_project.windows[i.cached_index].wrapped.y_size;
				if(i.fill_x) {
					if(glue_horizontal) {
						res.other = measure_result::special::space_consumer;
						res.x_space = 0;
					} else {
						res.x_space = int16_t(max_crosswise);
					}
				}
				if(i.fill_y) {
					if(!glue_horizontal) {
						res.other = measure_result::special::space_consumer;
						res.y_space = 0;
					} else {
						res.y_space = int16_t(max_crosswise);
					}
				}
				return res;
			}
		} else if(std::holds_alternative<layout_glue_t>(m)) {
			auto& i = std::get<layout_glue_t>(m);
			if(glue_horizontal) {
				switch(i.type) {
					case glue_type::standard: return measure_result{ i.amount, 0, measure_result::special::none };
					case glue_type::at_least: return measure_result{ i.amount, 0, measure_result::special::space_consumer };
					case glue_type::line_break: return measure_result{ 0, 0, measure_result::special::end_line };
					case glue_type::page_break: return measure_result{ 0, 0, measure_result::special::end_page };
					case glue_type::glue_don_t_break: return measure_result{ i.amount, 0, measure_result::special::no_break };
				}
			} else {
				switch(i.type) {
					case glue_type::standard: return measure_result{ 0, i.amount, measure_result::special::none };
					case glue_type::at_least: return measure_result{ 0, i.amount, measure_result::special::space_consumer };
					case glue_type::line_break: return measure_result{ 0, 0, measure_result::special::end_line };
					case glue_type::page_break: return measure_result{ 0, 0, measure_result::special::end_page };
					case glue_type::glue_don_t_break: return measure_result{ 0, i.amount, measure_result::special::no_break };
				}
			}
		} else if(std::holds_alternative<generator_t>(m)) {
			auto& i = std::get<generator_t>(m);
			for(auto& j : i.inserts) {
				update_cached_window(open_project, j.name, j.cached_index);
			}
			if(position.sub_index < int32_t(i.inserts.size()) && i.inserts[position.sub_index].cached_index != -1) {
				return measure_result{ open_project.windows[i.inserts[position.sub_index].cached_index].wrapped.x_size, open_project.windows[i.inserts[position.sub_index].cached_index].wrapped.y_size, measure_result::special::none };
			} else {
				return measure_result{ 0, 0, measure_result::special::none };
			}
		} else if(std::holds_alternative<sub_layout_t>(m)) {
			auto& i = std::get<sub_layout_t>(m);
			int32_t x = 0;
			int32_t y = 0;
			bool consume_fill = false;
			if(i.layout->size_x != -1)
				x = i.layout->size_x;
			else {
				if(glue_horizontal)
					consume_fill = true;
				else
					x = max_crosswise;
			}
			if(i.layout->size_y != -1)
				y = i.layout->size_y;
			else {
				if(!glue_horizontal)
					consume_fill = true;
				else
					y = max_crosswise;
			}
			return measure_result{ x, y, consume_fill ? measure_result::special::space_consumer : measure_result::special::none };
		}
		return measure_result{ 0, 0, measure_result::special::none };
	}
	void render_current(
		simple_fs::file_system const& fs,
		asvg::file_bank& svg_image_files,
                open_project_t& open_project,  template_project::project& open_templates, window_element_wrapper_t& window, int layer, float x, float y, int32_t width, int32_t height, color3f outline_color, float scale, int32_t layout_x, int32_t layout_y
        ) {
		if(!has_more())
			return;
		auto& m = backing[position.index];

		if(std::holds_alternative<layout_control_t>(m)) {
			auto& i = std::get<layout_control_t>(m);
			if(i.cached_index != -1) {
				if(i.fill_x)
					window.children[i.cached_index].x_size = int16_t(width);
				if(i.fill_y)
					window.children[i.cached_index].y_size = int16_t(height);

				if(i.absolute_position) {
					window.children[i.cached_index].x_pos = int16_t((layout_x + i.abs_x) * scale);
					window.children[i.cached_index].y_pos = int16_t((layout_y + i.abs_y) * scale);
					render_control(
                                                fs,
                                                svg_image_files,
                                                open_project, open_templates,
                                                window.children[i.cached_index], layout_x + i.abs_x, layout_y + i.abs_y, scale
                                        );
				} else {
					window.children[i.cached_index].x_pos = int16_t(x * scale);
					window.children[i.cached_index].y_pos = int16_t(y * scale);
					render_control(
                                                fs,
                                                svg_image_files,open_project, open_templates, window.children[i.cached_index], x, y, scale);
				}
			}
		} else if(std::holds_alternative<layout_window_t>(m)) {
			auto& i = std::get<layout_window_t>(m);
			if(i.cached_index != -1) {
				auto in_x = open_project.windows[i.cached_index].wrapped.x_size;
				auto in_y = open_project.windows[i.cached_index].wrapped.y_size;
				if(i.fill_x)
					open_project.windows[i.cached_index].wrapped.x_size = int16_t(width);
				if(i.fill_y)
					open_project.windows[i.cached_index].wrapped.y_size = int16_t(height);

				if(i.absolute_position) {
					render_window(fs, svg_image_files, open_project, open_templates, open_project.windows[i.cached_index], x, y, false, scale);
				} else {
					render_window(fs, svg_image_files, open_project, open_templates, open_project.windows[i.cached_index], x, y, false, scale);
				}
				open_project.windows[i.cached_index].wrapped.x_size = in_x;
				open_project.windows[i.cached_index].wrapped.y_size = in_y;
			}
		} else if(std::holds_alternative<layout_glue_t>(m)) {

		} else if(std::holds_alternative<generator_t>(m)) {
			auto& i = std::get<generator_t>(m);
			for(auto& j : i.inserts) {
				update_cached_window(open_project, j.name, j.cached_index);
			}
			if(position.sub_index < int32_t(i.inserts.size()) && i.inserts[position.sub_index].cached_index != -1) {
				render_window(fs, svg_image_files, open_project, open_templates, open_project.windows[i.inserts[position.sub_index].cached_index], x, y, false, scale);
			}
		} else if(std::holds_alternative<sub_layout_t>(m)) {
			auto& i = std::get<sub_layout_t>(m);
			render_layout(fs, svg_image_files, open_project, open_templates, window, *(i.layout), layer + 1, x, y, width, height, outline_color, scale);
		}
	}
	void move_position(int32_t n) {
		while(n > 0 && has_more()) {
			if(std::holds_alternative<generator_t>(backing[position.index])) {
				auto& g = std::get<generator_t>(backing[position.index]);
				auto sub_count = g.inserts.size();
				if(n >= int32_t(sub_count - position.sub_index)) {
					n -= int32_t(sub_count - position.sub_index);
					position.sub_index = 0;
					++position.index;
				} else {
					position.sub_index += n;
					n = 0;
				}
			} else {
				++position.index;
				--n;
			}
		}
		while(n < 0 && position.index >= 0) {
			if(position.index >= int32_t(backing.size())) {
				position.index = int32_t(backing.size()) - 1;
				if(backing.size() > 0 && std::holds_alternative<generator_t>(backing[position.index])) {
					auto& g = std::get<generator_t>(backing[position.index]);
					position.sub_index = std::max(int32_t(g.inserts.size()) - 1, 0);
				}
				++n;
			} else if(std::holds_alternative<generator_t>(backing[position.index])) {
				auto& g = std::get<generator_t>(backing[position.index]);
				if(-n > position.sub_index) {
					n += (position.sub_index + 1);
					--position.index;
				} else {
					position.sub_index += n;
					n = 0;
					break; // don't reset sub index
				}
			} else {
				--position.index;
				++n;
			}

			if(position.index < 0) {
				position.sub_index = 0;
				position.index = 0; return;
			}
			if(std::holds_alternative<generator_t>(backing[position.index])) {
				auto& g = std::get<generator_t>(backing[position.index]);
				position.sub_index = std::max(int32_t(g.inserts.size()) - 1, 0);
			}
		}
	}
	bool has_more() {
		return position.index < int32_t(backing.size());
	}
	void reset() {
		position.index = 0;
		position.sub_index = 0;
	}
};

index_result nth_layout_child(layout_level_t& m, int32_t index) {
	int32_t i = 0;
	for(auto& li : m.contents) {
		if(std::holds_alternative<generator_t>(li)) {
			auto& g = std::get<generator_t>(li);
			if(int32_t(i + g.inserts.size()) >= index) {
				return index_result{ &li, index - i };
			}
			i += int32_t(g.inserts.size());
		} else {
			if(i == index)
				return index_result{ &li, 0 };
			++i;
		}
	}
	return index_result{ nullptr, 0 };
}


struct layout_box {
	uint16_t x_dim = 0;
	uint16_t y_dim = 0;
	uint16_t item_count = 0;
	uint16_t space_conumer_count = 0;
	uint16_t non_glue_count = 0;
	bool end_page = false;
};

layout_box measure_horizontal_box(open_project_t& open_project,  template_project::project& open_templates, window_element_wrapper_t& win, layout_iterator& source, int32_t max_x, int32_t max_y) {
	layout_box result{ };

	auto initial_pos = source.position;

	while(source.has_more()) {
		auto m_result = source.measure_current(open_project, open_templates, win, true, max_y, source.position == initial_pos);
		bool is_glue = source.current_is_glue();
		int32_t xdtemp = result.x_dim;
		bool fits = ((m_result.x_space + result.x_dim) <= max_x) || (source.position == initial_pos) || is_glue;

		if(fits) {
			result.x_dim = std::min(uint16_t(m_result.x_space + result.x_dim), uint16_t(max_x));
			int32_t xdtemp2 = result.x_dim;
			result.y_dim = std::max(result.y_dim, uint16_t(m_result.y_space));
			if(m_result.other == measure_result::special::space_consumer) {
				++result.space_conumer_count;
			}
			++result.item_count;
			if(!is_glue)
				++result.non_glue_count;
			if(m_result.other == measure_result::special::end_page) {
				result.end_page = true;
				source.move_position(1);
				break;
			}
			if(m_result.other == measure_result::special::end_line) {
				source.move_position(1);
				break;
			}
		} else {
			break;
		}

		source.move_position(1);
	}

	int32_t rollback_count = 0;
	auto rollback_end_pos = source.position;

	// rollback loop -- drop any items that were glued to the preivous item
	while(source.position > initial_pos) {
		source.move_position(-1);
		auto m_result = source.measure_current(open_project, open_templates, win, true, max_y, source.position == initial_pos);
		if(m_result.other != measure_result::special::no_break) {
			source.move_position(1);
			break;
		}
		if(source.current_is_glue()) // don't break just before no break glue
			source.move_position(-1);
	}

	if(source.position > initial_pos && rollback_end_pos != (source.position)) { // non trivial rollback
		result = layout_box{ };
		auto new_end = source.position;
		source.position = initial_pos;

		// final measurement loop if rollback was non zero
		while(source.position < new_end) {
			auto m_result = source.measure_current(open_project, open_templates, win, true, max_y, source.position == initial_pos);
			bool is_glue = source.current_is_glue();

			result.x_dim = std::min(uint16_t(m_result.x_space + result.x_dim), uint16_t(max_x));
			result.y_dim = std::max(result.y_dim, uint16_t(m_result.y_space));
			if(m_result.other == measure_result::special::space_consumer) {
				++result.space_conumer_count;
			}
			if(!is_glue)
				++result.non_glue_count;
			++result.item_count;

			if(m_result.other == measure_result::special::end_page) {
				result.end_page = true;
			}

			source.move_position(1);
		}
	}

	return result;
}
layout_box measure_vertical_box(open_project_t& open_project,  template_project::project& open_templates, window_element_wrapper_t& win, layout_iterator& source, int32_t max_x, int32_t max_y) {
	layout_box result{ };

	auto initial_pos = source.position;

	while(source.has_more()) {
		auto m_result = source.measure_current(open_project, open_templates, win, false, max_x, source.position == initial_pos);
		bool is_glue = source.current_is_glue();
		bool fits = ((m_result.y_space + result.y_dim) <= max_y) || (source.position == initial_pos) || is_glue;

		if(fits) {
			result.y_dim = std::min(uint16_t(m_result.y_space + result.y_dim), uint16_t(max_y));
			result.x_dim = std::max(result.x_dim, uint16_t(m_result.x_space));
			if(m_result.other == measure_result::special::space_consumer) {
				++result.space_conumer_count;
			}
			++result.item_count;
			if(!is_glue)
				++result.non_glue_count;
			if(m_result.other == measure_result::special::end_page) {
				result.end_page = true;
				source.move_position(1);
				break;
			}
			if(m_result.other == measure_result::special::end_line) {
				source.move_position(1);
				break;
			}
		} else {
			break;
		}

		source.move_position(1);
	}

	int32_t rollback_count = 0;
	auto rollback_end_pos = source.position;

	// rollback loop -- drop any items that were glued to the preivous item
	while(source.position > initial_pos) {
		source.move_position(-1);
		auto m_result = source.measure_current(open_project, open_templates, win, false, max_x, source.position == initial_pos);
		if(m_result.other != measure_result::special::no_break) {
			source.move_position(1);
			break;
		}
		if(source.current_is_glue()) // don't break just before no break glue
			source.move_position(-1);
	}

	if(source.position > initial_pos && rollback_end_pos != (source.position)) { // non trivial rollback
		result = layout_box{ };
		auto new_end = source.position;
		source.position = initial_pos;

		// final measurement loop if rollback was non zero
		while(source.position < new_end) {
			auto m_result = source.measure_current(open_project, open_templates, win, false, max_x, source.position == initial_pos);
			bool is_glue = source.current_is_glue();

			result.y_dim = std::min(uint16_t(m_result.y_space + result.y_dim), uint16_t(max_y));
			result.x_dim = std::max(result.x_dim, uint16_t(m_result.x_space));
			if(m_result.other == measure_result::special::space_consumer) {
				++result.space_conumer_count;
			}
			if(!is_glue)
				++result.non_glue_count;
			++result.item_count;

			if(m_result.other == measure_result::special::end_page) {
				result.end_page = true;
			}

			source.move_position(1);
		}
	}

	return result;
}


GLint compile_shader(std::string_view source, GLenum type) {
	GLuint return_value = glCreateShader(type);

	if(return_value == 0) {
		perror("shader creation failed");
	}

	std::string s_source(source);
	GLchar const* texts[] = {
		"#version 140\r\n",
		"#extension GL_ARB_explicit_uniform_location : enable\r\n",
		"#extension GL_ARB_explicit_attrib_location : enable\r\n",
		"#extension GL_ARB_shader_subroutine : enable\r\n",
		"#define M_PI 3.1415926535897932384626433832795\r\n",
		"#define PI 3.1415926535897932384626433832795\r\n",
		s_source.c_str()
	};
	glShaderSource(return_value, 7, texts, nullptr);
	glCompileShader(return_value);

	GLint result;
	glGetShaderiv(return_value, GL_COMPILE_STATUS, &result);
	if(result == GL_FALSE) {
		GLint log_length = 0;
		glGetShaderiv(return_value, GL_INFO_LOG_LENGTH, &log_length);

		auto log = std::unique_ptr<char[]>(new char[static_cast<size_t>(log_length)]);
		GLsizei written = 0;
		glGetShaderInfoLog(return_value, log_length, &written, log.get());
		auto error = std::string("Shader failed to compile:\n") + log.get();
                perror(error.c_str());
	}
	return return_value;
}

GLuint create_program(std::string_view vertex_shader, std::string_view fragment_shader) {
	GLuint return_value = glCreateProgram();
	if(return_value == 0) {
		perror("OpenGL program creation failed");
	}

	auto v_shader = compile_shader(vertex_shader, GL_VERTEX_SHADER);
	auto f_shader = compile_shader(fragment_shader, GL_FRAGMENT_SHADER);

	glAttachShader(return_value, v_shader);
	glAttachShader(return_value, f_shader);
	glLinkProgram(return_value);

	GLint result;
	glGetProgramiv(return_value, GL_LINK_STATUS, &result);
	if(result == GL_FALSE) {
		GLint logLen;
		glGetProgramiv(return_value, GL_INFO_LOG_LENGTH, &logLen);

		char* log = new char[static_cast<size_t>(logLen)];
		GLsizei written;
		glGetProgramInfoLog(return_value, logLen, &written, log);
		auto err = std::string("Program failed to link:\n") + log;
		perror(err.c_str());
	}

	glDeleteShader(v_shader);
	glDeleteShader(f_shader);

	return return_value;
}



static GLfloat global_square_data[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f };
static GLfloat global_square_right_data[] = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f };
static GLfloat global_square_left_data[] = { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f };
static GLfloat global_square_flipped_data[] = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f };
static GLfloat global_square_right_flipped_data[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f };
static GLfloat global_square_left_flipped_data[] = { 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };

static GLuint ui_shader_program = 0;

void use_program(int display_w, int display_h) {
        glUseProgram(ui_shader_program);
        glUniform1i(glGetUniformLocation(ui_shader_program, "texture_sampler"), 0);
        glUniform1f(glGetUniformLocation(ui_shader_program, "screen_width"), float(display_w));
        glUniform1f(glGetUniformLocation(ui_shader_program, "screen_height"), float(display_h));
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void load_shaders() {

	std::string_view fx_str =
		"in vec2 tex_coord;\n"
		"out vec4 frag_color;\n"
		"uniform sampler2D texture_sampler;\n"
		"uniform vec4 d_rect;\n"
		"uniform float border_size;\n"
		"uniform float grid_size;\n"
		"uniform vec2 grid_off;\n"
		"uniform vec3 inner_color;\n"
		"uniform uvec2 subroutines_index;\n"

		"vec4 empty_rect(vec2 tc) {\n"
			"float realx = tc.x * d_rect.z;\n"
			"float realy = tc.y * d_rect.w;\n"
			"if(realx <= 2.5 || realy <= 2.5 || realx >= (d_rect.z -2.5) || realy >= (d_rect.w -2.5))\n"
				"return vec4(inner_color.r, inner_color.g, inner_color.b, 1.0f);\n"
			"return vec4(inner_color.r, inner_color.g, inner_color.b, 0.25f);\n"
		"}\n"
		"vec4 hollow_rect(vec2 tc) {\n"
			"float realx = tc.x * d_rect.z;\n"
			"float realy = tc.y * d_rect.w;\n"
			"if(realx <= 4.5 || realy <= 4.5 || realx >= (d_rect.z -4.5) || realy >= (d_rect.w -4.5))\n"
			"return vec4(inner_color.r, inner_color.g, inner_color.b, 1.0f);\n"
			"return vec4(inner_color.r, inner_color.g, inner_color.b, 0.0f);\n"
		"}\n"
		"vec4 grid_texture(vec2 tc) {\n"
			"float realx = grid_off.x + tc.x * d_rect.z;\n"
			"float realy = grid_off.y + tc.y * d_rect.w;\n"
			"if(mod(realx, grid_size) < 1.0f || mod(realy, grid_size) < 1.0f)\n"
				"return vec4(1.0f, 1.0f, 1.0f, 0.1f);\n"
			"return vec4(0.0f, 0.0f, 0.0f, 0.0f);\n"
		"}\n"
		"vec4 direct_texture(vec2 tc) {\n"
			"float realx = tc.x * d_rect.z;\n"
			"float realy = tc.y * d_rect.w;\n"
			// "if(realx <= 2.5 || realy <= 2.5 || realx >= (d_rect.z -2.5) || realy >= (d_rect.w -2.5))\n"
				// "return vec4(inner_color.r, inner_color.g, inner_color.b, 1.0f);\n"
			"\treturn texture(texture_sampler, tc);\n"
		"}\n"
		"vec4 frame_stretch(vec2 tc) {\n"
			"float realx = tc.x * d_rect.z;\n"
			"float realy = tc.y * d_rect.w;\n"
			"if(realx <= 2.5 || realy <= 2.5 || realx >= (d_rect.z -2.5) || realy >= (d_rect.w -2.5))\n"
				"return vec4(inner_color.r, inner_color.g, inner_color.b, 1.0f);\n"
			"vec2 tsize = textureSize(texture_sampler, 0);\n"
			"float xout = 0.0;\n"
			"float yout = 0.0;\n"
			"if(realx <= border_size * grid_size)\n"
				"xout = realx / (tsize.x * grid_size);\n"
			"else if(realx >= (d_rect.z - border_size * grid_size))\n"
				"xout = (1.0 - border_size / tsize.x) + (border_size * grid_size - (d_rect.z - realx)) / (tsize.x * grid_size);\n"
			"else\n"
				"xout = border_size / tsize.x + (1.0 - 2.0 * border_size / tsize.x) * (realx - border_size * grid_size) / (d_rect.z * 2.0 * border_size * grid_size);\n"
			"if(realy <= border_size * grid_size)\n"
				"yout = realy / (tsize.y * grid_size);\n"
			"else if(realy >= (d_rect.w - border_size * grid_size))\n"
				"yout = (1.0 - border_size / tsize.y) + (border_size * grid_size - (d_rect.w - realy)) / (tsize.y * grid_size);\n"
			"else\n"
				"yout = border_size / tsize.y + (1.0 - 2.0 * border_size / tsize.y) * (realy - border_size * grid_size) / (d_rect.w * 2.0 * border_size * grid_size);\n"
			"return texture(texture_sampler, vec2(xout, yout));\n"
		"}\n"
		"vec4 coloring_function(vec2 tc) {\n"
			"\tswitch(int(subroutines_index.x)) {\n"
				"\tcase 1: return empty_rect(tc);\n"
				"\tcase 2: return direct_texture(tc);\n"
				"\tcase 3: return frame_stretch(tc);\n"
				"\tcase 4: return grid_texture(tc);\n"
				"\tcase 5: return hollow_rect(tc);\n"
				"\tdefault: break;\n"
			"\t}\n"
			"\treturn vec4(1.0f,1.0f,1.0f,1.0f);\n"
		"}\n"
		"void main() {\n"
			"\tfrag_color = coloring_function(tex_coord);\n"
		"}";
	std::string_view vx_str =
		"layout (location = 0) in vec2 vertex_position;\n"
		"layout (location = 1) in vec2 v_tex_coord;\n"
		"out vec2 tex_coord;\n"
		"uniform float screen_width;\n"
		"uniform float screen_height;\n"
		"uniform vec4 d_rect;\n"
		"void main() {\n"
			"\tgl_Position = vec4(\n"
				"\t\t-1.0 + (2.0 * ((vertex_position.x * d_rect.z)  + d_rect.x) / screen_width),\n"
				"\t\t 1.0 - (2.0 * ((vertex_position.y * d_rect.w)  + d_rect.y) / screen_height),\n"
				"\t\t0.0, 1.0);\n"
			"\ttex_coord = v_tex_coord;\n"
		"}";

	ui_shader_program = create_program(vx_str, fx_str);
}

static GLuint global_square_vao = 0;
static GLuint global_square_buffer = 0;
static GLuint global_square_right_buffer = 0;
static GLuint global_square_left_buffer = 0;
static GLuint global_square_flipped_buffer = 0;
static GLuint global_square_right_flipped_buffer = 0;
static GLuint global_square_left_flipped_buffer = 0;

static GLuint sub_square_buffers[64] = { 0 };

void load_global_squares() {
	glGenBuffers(1, &global_square_buffer);

	// Populate the position buffer
	glBindBuffer(GL_ARRAY_BUFFER, global_square_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, global_square_data, GL_STATIC_DRAW);

	glGenVertexArrays(1, &global_square_vao);
	glBindVertexArray(global_square_vao);
	glEnableVertexAttribArray(0); // position
	glEnableVertexAttribArray(1); // texture coordinates

	glBindVertexBuffer(0, global_square_buffer, 0, sizeof(GLfloat) * 4);

	glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);					 // position
	glVertexAttribFormat(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 2);	// texture coordinates
	glVertexAttribBinding(0, 0);											// position -> to array zero
	glVertexAttribBinding(1, 0);											 // texture coordinates -> to array zero

	glGenBuffers(1, &global_square_left_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, global_square_left_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, global_square_left_data, GL_STATIC_DRAW);

	glGenBuffers(1, &global_square_right_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, global_square_right_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, global_square_right_data, GL_STATIC_DRAW);

	glGenBuffers(1, &global_square_right_flipped_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, global_square_right_flipped_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, global_square_right_flipped_data, GL_STATIC_DRAW);

	glGenBuffers(1, &global_square_left_flipped_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, global_square_left_flipped_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, global_square_left_flipped_data, GL_STATIC_DRAW);

	glGenBuffers(1, &global_square_flipped_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, global_square_flipped_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, global_square_flipped_data, GL_STATIC_DRAW);

	glGenBuffers(64, sub_square_buffers);
	for(uint32_t i = 0; i < 64; ++i) {
		glBindBuffer(GL_ARRAY_BUFFER, sub_square_buffers[i]);

		float const cell_x = static_cast<float>(i & 7) / 8.0f;
		float const cell_y = static_cast<float>((i >> 3) & 7) / 8.0f;

		GLfloat global_sub_square_data[] = { 0.0f, 0.0f, cell_x, cell_y, 0.0f, 1.0f, cell_x, cell_y + 1.0f / 8.0f, 1.0f, 1.0f,
			cell_x + 1.0f / 8.0f, cell_y + 1.0f / 8.0f, 1.0f, 0.0f, cell_x + 1.0f / 8.0f, cell_y };

		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, global_sub_square_data, GL_STATIC_DRAW);
	}
}


void render_textured_rect(color3f color, float ix, float iy, int32_t iwidth, int32_t iheight, GLuint texture_handle) {
	float x = float(ix);
	float y = float(iy);
	float width = float(iwidth);
	float height = float(iheight);

        assert_no_errors();

	glBindVertexArray(global_square_vao);
        assert_no_errors();

	glBindVertexBuffer(0, global_square_buffer, 0, sizeof(GLfloat) * 4);
        assert_no_errors();

	glUniform4f(glGetUniformLocation(ui_shader_program, "d_rect"), x, y, width, height);
	glUniform3f(glGetUniformLocation(ui_shader_program, "inner_color"), color.r, color.g, color.b);
        assert_no_errors();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_handle);
        assert_no_errors();

	GLuint subroutines[2] = { 2, 0 };
	glUniform2ui(glGetUniformLocation(ui_shader_program, "subroutines_index"), subroutines[0], subroutines[1]);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call
        assert_no_errors();

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        assert_no_errors();
}
void render_stretch_textured_rect(color3f color, float ix, float iy, float ui_scale, int32_t iwidth, int32_t iheight, float border_size, GLuint texture_handle) {
	float x = float(ix);
	float y = float(iy);
	float width = float(iwidth);
	float height = float(iheight);

	glBindVertexArray(global_square_vao);

	glBindVertexBuffer(0, global_square_buffer, 0, sizeof(GLfloat) * 4);

	glUniform1f(glGetUniformLocation(ui_shader_program, "border_size"), border_size);
	glUniform1f(glGetUniformLocation(ui_shader_program, "grid_size"), ui_scale);
	glUniform4f(glGetUniformLocation(ui_shader_program, "d_rect"), x, y, width, height);
	glUniform3f(glGetUniformLocation(ui_shader_program, "inner_color"), color.r, color.g, color.b);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_handle);

	GLuint subroutines[2] = { 3, 0 };
	glUniform2ui(glGetUniformLocation(ui_shader_program, "subroutines_index"), subroutines[0], subroutines[1]);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        assert_no_errors();
}
void render_empty_rect(color3f color, float ix, float iy, int32_t iwidth, int32_t iheight) {
	float x = float(ix);
	float y = float(iy);
	float width = float(iwidth);
	float height = float(iheight);

	glBindVertexArray(global_square_vao);

	glBindVertexBuffer(0, global_square_buffer, 0, sizeof(GLfloat) * 4);

	glUniform4f(glGetUniformLocation(ui_shader_program, "d_rect"), x, y, width, height);
	glUniform3f(glGetUniformLocation(ui_shader_program, "inner_color"), color.r, color.g, color.b);

	GLuint subroutines[2] = { 1, 0 };
	glUniform2ui(glGetUniformLocation(ui_shader_program, "subroutines_index"), subroutines[0], subroutines[1]);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        assert_no_errors();
}
void render_hollow_rect(color3f color, float ix, float iy, int32_t iwidth, int32_t iheight) {
	/*
	float x = float(ix);
	float y = float(iy);
	float width = float(iwidth);
	float height = float(iheight);

	glBindVertexArray(global_square_vao);

	glBindVertexBuffer(0, global_square_buffer, 0, sizeof(GLfloat) * 4);

	glUniform4f(glGetUniformLocation(ui_shader_program, "d_rect"), x, y, width, height);
	glUniform3f(glGetUniformLocation(ui_shader_program, "inner_color"), color.r, color.g, color.b);

	GLuint subroutines[2] = { 5, 0 };
	glUniform2ui(glGetUniformLocation(ui_shader_program, "subroutines_index"), subroutines[0], subroutines[1]);
	//glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 2, subroutines); // must set all subroutines in one call

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        assert_no_errors();
	*/
}

void render_layout_rect(color3f outline_color, float ix, float iy, int32_t iwidth, int32_t iheight) {
	// render_empty_rect(outline_color * 0.5f, ix, iy, iwidth, iheight);
	// render_hollow_rect(outline_color, ix, iy, iwidth, iheight);
}

void render_layout(
        simple_fs::file_system const& fs,
        asvg::file_bank& svg_image_files,
        open_project_t& open_project,
        template_project::project& open_templates,
        window_element_wrapper_t& window,
        layout_level_t& layout,
        int layer,
        float x, float y, int32_t width, int32_t height, color3f outline_color, float scale
) {
	auto base_x_size = layout.size_x != -1 ? int32_t(layout.size_x) : width;
	auto base_y_size = layout.size_y != -1 ? int32_t(layout.size_y) : height;
	auto top_margin = int32_t(layout.margin_top);
	auto bottom_margin = layout.margin_bottom != -1 ? int32_t(layout.margin_bottom) : top_margin;
	auto left_margin = layout.margin_left != -1 ? int32_t(layout.margin_left) : bottom_margin;
	auto right_margin = layout.margin_right != -1 ? int32_t(layout.margin_right) : left_margin;
	auto effective_x_size = base_x_size - (left_margin + right_margin);
	auto effective_y_size = base_y_size - (top_margin + bottom_margin);

	auto id = layout.template_id;
	if(id == -1 && window.wrapped.template_id != -1) {
		id = open_templates.window_t[window.wrapped.template_id].layout_region_definition;
	}
	if(id != -1) {
		auto bg = open_templates.layout_region_t[id].bg;
		if(bg != -1) {
			render_textured_rect(color3f{ 0.f, 0.f, 0.f },
				x * scale,
				y * scale,
				std::max(1, int32_t(base_x_size * scale)),
				std::max(1, int32_t(base_y_size * scale)),
				open_templates.backgrounds[bg].renders.get_render(fs, svg_image_files, base_x_size / float(open_project.grid_size), base_y_size / float(open_project.grid_size), open_project.grid_size, 2.0f));
		}
	}

	if(layout.paged) {
		effective_y_size -= int32_t(2 * open_project.grid_size);
	}

	if(layout.open_in_ui) {
		render_layout_rect(outline_color * 2.f * layer, ((x + left_margin) * scale), ((y + top_margin) * scale), std::max(1, int32_t(effective_x_size * scale)), std::max(1, int32_t(effective_y_size * scale)));
	} else {
		render_layout_rect(outline_color * 0.5f * layer, ((x + left_margin) * scale), ((y + top_margin) * scale), std::max(1, int32_t(effective_x_size * scale)), std::max(1, int32_t(effective_y_size * scale)));
	}

	auto& lvl = layout;
	switch(layout.type) {
		case layout_type::single_horizontal:
		{
			int32_t index_start = 0;
			layout_iterator it(lvl.contents);
			it.move_position(index_start);

			auto start_pos = it.position;
			auto box = measure_horizontal_box(open_project, open_templates, window, it, effective_x_size, effective_y_size);
			it.position = start_pos;

			int32_t space_used = box.x_dim;
			int32_t fill_consumer_count = box.space_conumer_count;
			// place / render

			int32_t extra_runlength = int32_t(effective_x_size - space_used);
			int32_t per_fill_consumer = fill_consumer_count != 0 ? (extra_runlength / fill_consumer_count) : 0;
			int32_t extra_lead = 0;
			switch(lvl.line_alignment) {
				case layout_line_alignment::leading: break;
				case layout_line_alignment::trailing: extra_lead = extra_runlength - fill_consumer_count * per_fill_consumer; break;
				case layout_line_alignment::centered: extra_lead = (extra_runlength - fill_consumer_count * per_fill_consumer) / 2;  break;
			}

			space_used = x + extra_lead + left_margin;
			bool alternate = true;
			for(uint16_t i = 0; i < box.item_count; ++i) {
				auto mr = it.measure_current(open_project, open_templates, window, true, effective_y_size, i == 0);
				int32_t yoff = 0;
				int32_t xoff = space_used;
				switch(lvl.line_internal_alignment) {
					case layout_line_alignment::leading: yoff = y + top_margin; break;
					case layout_line_alignment::trailing: yoff = y + top_margin + effective_y_size - mr.y_space; break;
					case layout_line_alignment::centered: yoff = y + top_margin + (effective_y_size - mr.y_space) / 2;  break;
				}

				it.render_current(fs, svg_image_files, open_project, open_templates, window, layer, xoff, yoff, mr.x_space + (mr.other == measure_result::special::space_consumer ? per_fill_consumer : 0), mr.y_space, outline_color, scale, x, y);
				it.move_position(1);

				space_used += mr.x_space;
				if(mr.other == measure_result::special::space_consumer) {
					space_used += per_fill_consumer;
				}
			}
		} break;
		case layout_type::single_vertical:
		{
			int32_t index_start = 0;

			layout_iterator it(lvl.contents);
			it.move_position(index_start);

			auto start_pos = it.position;
			auto box = measure_vertical_box(open_project, open_templates, window, it, effective_x_size, effective_y_size);
			it.position = start_pos;

			int32_t space_used = box.y_dim;
			int32_t fill_consumer_count = box.space_conumer_count;
			// place / render

			int32_t extra_runlength = int32_t(effective_y_size - space_used);
			int32_t per_fill_consumer = fill_consumer_count != 0 ? (extra_runlength / fill_consumer_count) : 0;
			int32_t extra_lead = 0;
			switch(lvl.line_alignment) {
				case layout_line_alignment::leading: break;
				case layout_line_alignment::trailing: extra_lead = extra_runlength - fill_consumer_count * per_fill_consumer; break;
				case layout_line_alignment::centered: extra_lead = (extra_runlength - fill_consumer_count * per_fill_consumer) / 2;  break;
			}

			space_used = y + extra_lead + top_margin;
			bool alternate = true;
			for(uint16_t i = 0; i < box.item_count; ++i) {
				auto mr = it.measure_current(open_project, open_templates, window, false, effective_x_size, i == 0);

				int32_t xoff = 0;
				int32_t yoff = space_used;
				switch(lvl.line_internal_alignment) {
					case layout_line_alignment::leading: xoff = x + left_margin; break;
					case layout_line_alignment::trailing: xoff = x + left_margin + effective_x_size - mr.x_space; break;
					case layout_line_alignment::centered: xoff = x + left_margin + (effective_x_size - mr.x_space) / 2;  break;
				}

				it.render_current(fs, svg_image_files, open_project, open_templates, window, layer, xoff, yoff, mr.x_space, mr.y_space + (mr.other == measure_result::special::space_consumer ? per_fill_consumer : 0), outline_color, scale, x, y);
				it.move_position(1);

				space_used += mr.y_space;
				if(mr.other == measure_result::special::space_consumer) {
					space_used += per_fill_consumer;
				}
			}
		} break;
		case layout_type::overlapped_horizontal:
		{
			layout_iterator place_it(lvl.contents);
			int32_t index_start = 0;

			auto pre_pos = place_it.position;
			auto box = measure_horizontal_box(open_project, open_templates, window, place_it, std::numeric_limits<int32_t>::max(), effective_y_size);
			place_it.position = pre_pos;

			int32_t space_used = box.x_dim;
			int32_t fill_consumer_count = box.space_conumer_count;
			int32_t non_glue_count = box.non_glue_count;

			int32_t extra_runlength = std::max(0, int32_t(effective_x_size - space_used));
			int32_t per_fill_consumer = fill_consumer_count != 0 ? (extra_runlength / fill_consumer_count) : 0;
			int32_t extra_lead = 0;
			switch(lvl.line_alignment) {
				case layout_line_alignment::leading: break;
				case layout_line_alignment::trailing: extra_lead = extra_runlength - fill_consumer_count * per_fill_consumer; break;
				case layout_line_alignment::centered: extra_lead = (extra_runlength - fill_consumer_count * per_fill_consumer) / 2;  break;
			}
			int32_t overlap_subtraction = (non_glue_count > 1 && space_used > effective_x_size) ? int32_t(space_used - effective_x_size) / (non_glue_count - 1) : 0;
			space_used = x + extra_lead + left_margin;

			bool page_first = true;
			bool alternate = true;
			while(place_it.has_more()) {
				auto mr = place_it.measure_current(open_project, open_templates, window, true, effective_y_size, page_first);
				int32_t yoff = 0;
				int32_t xoff = space_used;
				switch(lvl.line_internal_alignment) {
					case layout_line_alignment::leading: yoff = y + top_margin; break;
					case layout_line_alignment::trailing: yoff = y + top_margin + effective_y_size - mr.y_space; break;
					case layout_line_alignment::centered: yoff = y + top_margin + (effective_y_size - mr.y_space) / 2;  break;
				}
				bool was_abs = false;
				if(std::holds_alternative< layout_control_t>(lvl.contents[place_it.position.index])) {
					auto& i = std::get<layout_control_t>(lvl.contents[place_it.position.index]);
					was_abs = i.absolute_position;
				} else if(std::holds_alternative< layout_window_t>(lvl.contents[place_it.position.index])) {
					auto& i = std::get<layout_window_t>(lvl.contents[place_it.position.index]);
					was_abs = i.absolute_position;
				}
				place_it.render_current(fs, svg_image_files, open_project, open_templates, window, layer, xoff, yoff, mr.x_space + (mr.other == measure_result::special::space_consumer ? per_fill_consumer : 0), mr.y_space, outline_color, scale, x, y);

				if(!place_it.current_is_glue()) {
					page_first = false;
				}

				space_used += mr.x_space;
				if(mr.other == measure_result::special::space_consumer) {
					space_used += per_fill_consumer;
				}
				if(!place_it.current_is_glue() && !was_abs)
					space_used -= overlap_subtraction;

				place_it.move_position(1);
			}
		} break;
		case layout_type::overlapped_vertical:
		{
			layout_iterator place_it(lvl.contents);
			int32_t index_start = 0;

			place_it.move_position(index_start);
			auto pre_pos = place_it.position;
			auto box = measure_horizontal_box(open_project, open_templates, window, place_it, effective_x_size, std::numeric_limits<int32_t>::max());
			place_it.position = pre_pos;

			int32_t space_used = box.y_dim;
			int32_t fill_consumer_count = box.space_conumer_count;
			int32_t non_glue_count = box.non_glue_count;

			int32_t extra_runlength = std::max(0, int32_t(effective_y_size - space_used));
			int32_t per_fill_consumer = fill_consumer_count != 0 ? (extra_runlength / fill_consumer_count) : 0;
			int32_t extra_lead = 0;
			switch(lvl.line_alignment) {
				case layout_line_alignment::leading: break;
				case layout_line_alignment::trailing: extra_lead = extra_runlength - fill_consumer_count * per_fill_consumer; break;
				case layout_line_alignment::centered: extra_lead = (extra_runlength - fill_consumer_count * per_fill_consumer) / 2;  break;
			}
			int32_t overlap_subtraction = (non_glue_count > 1 && space_used > effective_y_size) ? int32_t(space_used - effective_y_size) / (non_glue_count - 1) : 0;
			space_used = y + extra_lead + top_margin;

			bool page_first = true;
			bool alternate = true;
			while(place_it.has_more()) {
				auto mr = place_it.measure_current(open_project, open_templates, window, false, effective_x_size, page_first);
				int32_t xoff = 0;
				int32_t yoff = space_used;
				switch(lvl.line_internal_alignment) {
					case layout_line_alignment::leading: xoff = x + left_margin; break;
					case layout_line_alignment::trailing: xoff = x + left_margin + effective_x_size - mr.x_space; break;
					case layout_line_alignment::centered: xoff = x + left_margin + (effective_x_size - mr.x_space) / 2;  break;
				}
				bool was_abs = false;
				if(std::holds_alternative< layout_control_t>(lvl.contents[place_it.position.index])) {
					auto& i = std::get<layout_control_t>(lvl.contents[place_it.position.index]);
					was_abs = i.absolute_position;
				} else if(std::holds_alternative< layout_window_t>(lvl.contents[place_it.position.index])) {
					auto& i = std::get<layout_window_t>(lvl.contents[place_it.position.index]);
					was_abs = i.absolute_position;
				}

				place_it.render_current(fs, svg_image_files, open_project, open_templates, window, layer, xoff, yoff, mr.x_space, mr.y_space + (mr.other == measure_result::special::space_consumer ? per_fill_consumer : 0), outline_color, scale, x, y);

				if(!place_it.current_is_glue()) {
					page_first = false;
				}

				space_used += mr.y_space;
				if(mr.other == measure_result::special::space_consumer) {
					space_used += per_fill_consumer;
				}
				if(!place_it.current_is_glue() && !was_abs)
					space_used -= overlap_subtraction;

				place_it.move_position(1);
			}
		} break;
		case layout_type::mulitline_horizontal:
		{
			layout_iterator place_it(lvl.contents);
			int32_t index_start = 0;

			int32_t y_remaining = effective_y_size;
			bool first = true;
			while(place_it.has_more()) {
				auto pre_pos = place_it.position;

				auto box = measure_horizontal_box(open_project, open_templates, window, place_it, effective_x_size, y_remaining);
				assert(box.item_count > 0);
				if(box.y_dim > y_remaining && !first) { // end
					break;
				}

				place_it.position = pre_pos;
				bool alternate = true;

				int32_t extra_runlength = int32_t(effective_x_size - box.x_dim);
				int32_t per_fill_consumer = box.space_conumer_count != 0 ? (extra_runlength / box.space_conumer_count) : 0;
				int32_t extra_lead = 0;
				switch(lvl.line_alignment) {
					case layout_line_alignment::leading: break;
					case layout_line_alignment::trailing: extra_lead = extra_runlength - box.space_conumer_count * per_fill_consumer; break;
					case layout_line_alignment::centered: extra_lead = (extra_runlength - box.space_conumer_count * per_fill_consumer) / 2;  break;
				}
				auto space_used = x + extra_lead + left_margin;

				for(uint16_t i = 0; i < box.item_count; ++i) {
					auto mr = place_it.measure_current(open_project, open_templates, window, false, effective_x_size, i == 0);

					int32_t yoff = 0;
					int32_t xoff = space_used;
					switch(lvl.line_internal_alignment) {
						case layout_line_alignment::leading: yoff = y + top_margin + (effective_y_size - y_remaining); break;
						case layout_line_alignment::trailing: yoff = y + top_margin + (effective_y_size - y_remaining) + box.y_dim - mr.y_space; break;
						case layout_line_alignment::centered: yoff = y + top_margin + (effective_y_size - y_remaining) + (box.y_dim - mr.y_space) / 2;  break;
					}
					place_it.render_current(fs, svg_image_files, open_project, open_templates, window, layer, xoff, yoff, mr.x_space, mr.y_space + (mr.other == measure_result::special::space_consumer ? per_fill_consumer : 0), outline_color, scale, x, y);
					place_it.move_position(1);

					space_used += mr.x_space;
					if(mr.other == measure_result::special::space_consumer) {
						space_used += per_fill_consumer;
					}
				}

				y_remaining -= int32_t(box.y_dim + lvl.interline_spacing);
				if(y_remaining <= 0) {
					break;
				}
				if(box.end_page) {
					break;
				}
				first = false;
			}

		} break;
		case layout_type::multiline_vertical:
		{
			layout_iterator place_it(lvl.contents);
			int32_t index_start = 0;

			int32_t x_remaining = effective_x_size;
			bool first = true;
			while(place_it.has_more()) {
				auto pre_pos = place_it.position;

				auto box = measure_vertical_box(open_project, open_templates, window, place_it, x_remaining, effective_y_size);
				assert(box.item_count > 0);
				if(box.x_dim > x_remaining && !first) { // end
					break;
				}

				place_it.position = pre_pos;
				bool alternate = true;

				int32_t extra_runlength = int32_t(effective_y_size - box.y_dim);
				int32_t per_fill_consumer = box.space_conumer_count != 0 ? (extra_runlength / box.space_conumer_count) : 0;
				int32_t extra_lead = 0;
				switch(lvl.line_alignment) {
					case layout_line_alignment::leading: break;
					case layout_line_alignment::trailing: extra_lead = extra_runlength - box.space_conumer_count * per_fill_consumer; break;
					case layout_line_alignment::centered: extra_lead = (extra_runlength - box.space_conumer_count * per_fill_consumer) / 2;  break;
				}
				auto space_used = y + extra_lead + top_margin;

				for(uint16_t i = 0; i < box.item_count; ++i) {
					auto mr = place_it.measure_current(open_project, open_templates, window, false, effective_x_size, i == 0);

					int32_t xoff = 0;
					int32_t yoff = space_used;
					switch(lvl.line_internal_alignment) {
						case layout_line_alignment::leading: xoff = x + left_margin + (effective_x_size - x_remaining); break;
						case layout_line_alignment::trailing: xoff = x + left_margin + (effective_x_size - x_remaining) + box.x_dim - mr.x_space; break;
						case layout_line_alignment::centered: xoff = x + left_margin + (effective_x_size - x_remaining) + (box.x_dim - mr.x_space) / 2;  break;
					}
					place_it.render_current(fs, svg_image_files, open_project, open_templates, window, layer, xoff, yoff, mr.x_space + (mr.other == measure_result::special::space_consumer ? per_fill_consumer : 0), mr.y_space, outline_color, scale, x, y);
					place_it.move_position(1);

					space_used += mr.y_space;
					if(mr.other == measure_result::special::space_consumer) {
						space_used += per_fill_consumer;
					}
				}

				x_remaining -= int32_t(box.x_dim + lvl.interline_spacing);
				if(x_remaining <= 0)
					break;
				if(box.end_page)
					break;
				first = false;
			}

		} break;
	}
}