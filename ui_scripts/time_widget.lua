local ffi = require "ffi"

ffi.cdef[[
	int32_t get_current_game_speed();
	void inc_current_game_speed();
	void dec_current_game_speed();
	int64_t get_second();
	int64_t get_minute();
	int64_t get_hour();
	int64_t get_day();
	int64_t get_month();
	int64_t get_year();
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
	local time = string.format("YMD %d,%d,%d", ffi.C.get_year(), ffi.C.get_month(), ffi.C.get_day())
	return time
end

function UI_LOGIC.time_widget.main.time.text()
	local time = string.format("%02d:%02d:%02d", ffi.C.get_hour(), ffi.C.get_minute(), ffi.C.get_second())
	return time
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