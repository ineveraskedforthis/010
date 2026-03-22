local ffi = require "ffi"

ffi.cdef[[
	int32_t get_current_game_speed();
	void inc_current_game_speed();
	void dec_current_game_speed();
]]

UI_LOGIC.time_widget = {}
UI_LOGIC.time_widget.main = {}
UI_LOGIC.time_widget.main.decrease = {}
UI_LOGIC.time_widget.main.increase = {}
UI_LOGIC.time_widget.main.current = {}
UI_LOGIC.time_widget.main.turbo = {}
UI_LOGIC.time_widget.main.run = {}
UI_LOGIC.time_widget.main.date = {}
UI_LOGIC.time_widget.main.time = {}

function UI_LOGIC.time_widget.main.date.text()
	return "1"
end

function UI_LOGIC.time_widget.main.time.text()
	return "2"
end

function UI_LOGIC.time_widget.main.current.text()
	return tostring(ffi.C.get_current_game_speed())
end

function UI_LOGIC.time_widget.main.increase.left_click()
	ffi.C.inc_current_game_speed()
end

function UI_LOGIC.time_widget.main.decrease.left_click()
	ffi.C.dec_current_game_speed()
end