local ffi = require "ffi"

require "ui_scripts.time_widget"
require "codegen-lua.aui_window"


ffi.cdef[[
	int32_t load_aui(const char * filename);
	int32_t new_window_instance(int32_t aui_index, const char * project_name);
	void draw_aui(int32_t window_index, float x, float y);
]]

SCENE = {}

function SCENE.load()
        TIME_WIDGET_AUI = ffi.C.load_aui("time_widget")
        TIME_WIDGET = ffi.C.new_window_instance(TIME_WIDGET_AUI, "main")
end

function SCENE.set_positions(width, height)
        local w_width = AUI_WINDOW.get_width(TIME_WIDGET)
        -- local w_height = AUI_WINDOW.get_height(TIME_WIDGET)
        AUI_WINDOW.set_x(TIME_WIDGET, width - w_width)
end

function SCENE.update()

end

function SCENE.on_tile_click(tile)

end