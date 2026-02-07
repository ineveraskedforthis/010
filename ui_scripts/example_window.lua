local ffi = require "ffi"

UI_LOGIC.example_window = {}
UI_LOGIC.example_window.main = {}
UI_LOGIC.example_window.main.load_world = {}
UI_LOGIC.example_window.main.load_game = {}
UI_LOGIC.example_window.main.generate_world = {}
UI_LOGIC.example_window.main.open_settings = {}
UI_LOGIC.example_window.main.exit_game = {}

function UI_LOGIC.example_window.main.load_world.text()
	return "Images to world"
end
function UI_LOGIC.example_window.main.load_game.text()
	return "Load game"
end
function UI_LOGIC.example_window.main.generate_world.text()
	return "Generate world"
end
function UI_LOGIC.example_window.main.open_settings.text()
	return "Settings"
end
function UI_LOGIC.example_window.main.exit_game.text()
	return "Exit"
end