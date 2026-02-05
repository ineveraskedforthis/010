local ffi = require "ffi"

UI_STATE = {}


UI_STATE.example_window = {}

UI_STATE.example_window.main = {}

UI_STATE.example_window.main.load_world = {}



local load_text = "Load maps"
function UI_STATE.example_window.main.load_world.update()
	if UI_STATE.example_window.main.load_world.raw_text ~= load_text then

	end
end