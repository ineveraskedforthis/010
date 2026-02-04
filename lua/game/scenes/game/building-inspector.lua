local re = {}
local tabb = require "engine.table"
local trade_good = require "game.raws.raws-utils".trade_good
local use_case = require "game.raws.raws-utils".trade_good_use_case


local ib = require "game.scenes.game.widgets.inspector-redirect-buttons"
local list_widget = require "game.scenes.game.widgets.list-widget"
local economical = require "game.raws.values.economy"
local economic_effects = require "game.raws.effects.economy"
local dbm = require "game.economy.diet-breadth-model"
local production_method_utils = require "game.raws.production-methods"
local province_utils = require "game.entities.province".Province
local pop_utils = require "game.entities.pop".POP
local demography_effects = require "game.raws.effects.demography"

KEY_PRESS_MODIFIER = 1

local output_list_state = nil
local input_list_state = nil
local worker_list_state = nil

---@return Rect
local function get_main_panel()
	local fs = ui.fullscreen()
	local panel = fs:subrect(ut.BASE_HEIGHT * 2, 0, ut.BASE_HEIGHT * 16, ut.BASE_HEIGHT * 25, "left", "down")
	return panel
end

---Returns whether or not clicks on the planet can be registered.
---@return boolean
function re.mask()
	if ui.trigger(get_main_panel()) then
		return false
	else
		return true
	end
end

---@enum ESTATE_TAB
local ESTATE_TAB = {
	BUILDINGS = 1,
	MANAGEMENT = 2
}

---@type ESTATE_TAB
local selected_tab = ESTATE_TAB.BUILDINGS

local property_inventory_state = nil

---@param gam GameScene
function re.draw(gam)

	--- combining key presses for increments of 1, 5, 10, and 50
	KEY_PRESS_MODIFIER = 1
	if ui.is_key_held("lshift") or ui.is_key_held("rshift") then
		KEY_PRESS_MODIFIER = KEY_PRESS_MODIFIER * 2
	end
	if ui.is_key_held("lctrl") or ui.is_key_held("rctrl") then
		KEY_PRESS_MODIFIER = KEY_PRESS_MODIFIER * 4
	end

	local estate = gam.selected.estate
	if estate ~= nil and estate ~= INVALID_ID and DCON.dcon_estate_is_valid(estate - 1) then
		local panel = get_main_panel()
		ui.panel(panel)

		local owner = OWNER(estate)
		local province = ESTATE_PROVINCE(estate)
		local realm = PROVINCE_REALM(province)

		local top_line = panel:subrect(0, 0, panel.width, ut.BASE_HEIGHT, "right", "up")
		ui.panel(top_line)

		if owner == INVALID_ID then
			ui.text("Public estate of in " .. PROVINCE_NAME(province), top_line, "left", "center")
		else
			ui.text("Estate of " .. NAME(owner) .. " in " .. PROVINCE_NAME(province), top_line, "left", "center")
		end

		local close_button_rect = panel:subrect(0, 0, ut.BASE_HEIGHT, ut.BASE_HEIGHT, "right", "up")
		ib.icon_button_to_close(gam, close_button_rect)

		local owner_rect = top_line:copy()
		owner_rect.width = owner_rect.height
		owner_rect.y = owner_rect.y + ut.BASE_HEIGHT

		local button_owner_rect = owner_rect:copy()
		button_owner_rect.x = owner_rect.x + owner_rect.width
		button_owner_rect.width = ut.BASE_HEIGHT * 6

		if owner ~= INVALID_ID then
			-- target character
			ib.icon_button_to_character(gam, owner, owner_rect)
			ib.text_button_to_character(gam, owner, button_owner_rect, NAME(owner), NAME(owner) .. " owns this building.")
		else
			-- target realm if possible
			if realm ~= INVALID_ID then
				ib.icon_button_to_realm(gam, realm, owner_rect)
			else
				ut.render_icon(owner_rect, "world.png", 1, 1, 1, 1)
			end
			ib.text_button_to_province_tile(gam, DATA.province_get_center(province), button_owner_rect,
				"Public estates in" .. PROVINCE_NAME(province) .. ".")
		end


		local savings_rect = button_owner_rect:copy()
		savings_rect.x = button_owner_rect.x + button_owner_rect.width
		savings_rect.width = panel.width - owner_rect.width - button_owner_rect.width
		require "game.scenes.game.widgets.subsidy"(savings_rect, estate)

		local tabs_rect = panel:subrect(0, ut.BASE_HEIGHT * 2, panel.width, ut.BASE_HEIGHT, "left", "up")

		tabs_rect.width = tabs_rect.width / 2

		if ut.text_button(
			"Buildings",
			tabs_rect,
			"In this tab you can manage your workers and see production methods of your buildings",
			true,
			selected_tab == ESTATE_TAB.BUILDINGS
		) then
			selected_tab = ESTATE_TAB.BUILDINGS
		end

		tabs_rect.x = tabs_rect.x + tabs_rect.width

		if ut.text_button(
			"Inventory",
			tabs_rect,
			"In this tab you can manage your budget and inventory of your estate",
			true,
			selected_tab == ESTATE_TAB.MANAGEMENT
		) then
			selected_tab = ESTATE_TAB.MANAGEMENT
		end

		local management_rect = panel:subrect(0, ut.BASE_HEIGHT * 3, panel.width, panel.height - ut.BASE_HEIGHT * 3, "left", "up")

		management_rect:shrink(5)

		if selected_tab == ESTATE_TAB.BUILDINGS then
			---@type building_id
			local building = gam.selected.building

			local worker = DATA.employment_get_worker(DATA.get_employment_from_building(building))

			if worker == INVALID_ID then
				local buildings_list = management_rect:copy()
				buildings_list.height = buildings_list.height / 2

				local building_details = management_rect:copy()
				building_details.height = management_rect.height - buildings_list.height
				building_details.y = buildings_list.y + buildings_list.height

				buildings_list:shrink(5)
				ui.panel(buildings_list)
				require "game.scenes.game.widgets.buildings-list"(gam, buildings_list, estate)
				--- list all local buildings later
				require "game.scenes.game.widgets.building-hire"(building_details, building)
			else
				local buildings_list = management_rect:copy()
				buildings_list.height = buildings_list.height - ut.BASE_HEIGHT * 6

				local building_details = management_rect:copy()
				building_details.height = management_rect.height - buildings_list.height
				building_details.y = buildings_list.y + buildings_list.height

				buildings_list:shrink(5)
				ui.panel(buildings_list)
				require "game.scenes.game.widgets.buildings-list"(gam, buildings_list, estate)
				--- list all local buildings later
				require "game.scenes.game.widgets.building-details"(building_details, building)
			end


		else
			--- budget and inventory related decisions
			--- savings are already displayed:
			--- show inventory stats
			property_inventory_state = require "game.scenes.game.widgets.estate-inventory-list"(gam, management_rect, estate, property_inventory_state, nil, true)()
		end
	else
		gam.selected.estate = INVALID_ID
		gam.selected.building = INVALID_ID
	end
end

return re