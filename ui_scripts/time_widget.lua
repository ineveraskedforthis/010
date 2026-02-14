
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
        return "3"
end

-- function UI_LOGIC.time_widget.main.load_world.text()
-- 	return "Images to world"
-- end
-- function UI_LOGIC.time_widget.main.load_world.left_click()
-- 	ffi.C.change_scene(1)
-- end
-- function UI_LOGIC.time_widget.main.load_game.text()
-- 	return "Load game"
-- end
-- function UI_LOGIC.time_widget.main.generate_world.text()
-- 	return "Generate world"
-- end
-- function UI_LOGIC.time_widget.main.open_settings.text()
-- 	return "Settings"
-- end
-- function UI_LOGIC.time_widget.main.open_settings.left_click()
-- 	ffi.C.toggle_settings_window()
-- end
-- function UI_LOGIC.time_widget.main.exit_game.text()
-- 	return "Exit"
-- end