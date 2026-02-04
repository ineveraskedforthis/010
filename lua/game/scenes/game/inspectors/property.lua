local tabb = require "engine.table"

;

local ib = require "game.scenes.game.widgets.inspector-redirect-buttons"

local economy_values = require "game.raws.values.economy"

local inspector = {}

---@return Rect
local function get_main_panel()
	local fs = ui.fullscreen()
    return fs:subrect(ut.BASE_HEIGHT * 2, 0, ut.BASE_HEIGHT * 16, ut.BASE_HEIGHT * 25, "left", "down")
end

---Returns whether or not clicks on the planet can be registered.
---@return boolean
function inspector.mask()
	if ui.trigger(get_main_panel()) then
		return false
	else
		return true
	end
end

---@type TableState
local state = nil

local function init_state(base_unit)
    if state == nil then
        state = {
            header_height = base_unit,
            individual_height = base_unit,
            slider_level = 0,
            slider_width = base_unit,
            sorted_field = 1,
            sorting_order = true
        }
    else
        state.header_height = base_unit
        state.individual_height = base_unit
        state.slider_width = base_unit
    end
end

---comment
---@param gamescene GameScene
function inspector.draw(gamescene)
    local rect = get_main_panel()

	--- combining key presses for increments of 1, 5, 10, and 50
	KEY_PRESS_MODIFIER = 1
	if ui.is_key_held("lshift") or ui.is_key_held("rshift") then
		KEY_PRESS_MODIFIER = KEY_PRESS_MODIFIER * 2
	end
	if ui.is_key_held("lctrl") or ui.is_key_held("rctrl") then
		KEY_PRESS_MODIFIER = KEY_PRESS_MODIFIER * 4
	end

    ui.panel(rect)

    local province = gamescene.selected.province

    if province == nil then
        return
    end

    local base_unit = ut.BASE_HEIGHT
    init_state(base_unit)

    ---@type TableColumn<estate_id>[]
    local columns = {
        {
            header = "Name",
            ---@param v estate_id
            render_closure = function(rect, k, v)
                ib.text_button_to_estate(gamescene, v, INVALID_ID, rect, PROVINCE_NAME(ESTATE_PROVINCE(v)),
                    "Estates in " .. PROVINCE_NAME(ESTATE_PROVINCE(v)))
            end,
            width = base_unit * 6,
            ---@param v estate_id
            value = function(k, v)
                return PROVINCE_NAME(ESTATE_PROVINCE(v))
            end
        },
        {
            header = "Treasury",
            ---@param rect Rect
            ---@param v estate_id
            render_closure = function (rect, k, v)
                ut.money_entry_icon(DATA.estate_get_savings(v), rect, "Your local estates treasury")
            end,
            width = base_unit * 3,
            ---@param v estate_id
            value = function (k, v)
                return DATA.building_get_subsidy(v)
            end,
            active = true
        },
        {
            header = "Balance",
            ---@param v estate_id
            render_closure = function(rect, k, v)
                local bought_cost = 0
                local province = ESTATE_PROVINCE(v)
                DATA.for_each_trade_good(function (item)
                    local bought = DATA.estate_get_inventory_bought_last_tick(v, item)
                    bought_cost = bought_cost + bought * DATA.province_get_local_prices(province, item)
                end)
                local sold_cost = 0
                local province = ESTATE_PROVINCE(v)
                DATA.for_each_trade_good(function (item)
                    local sold = DATA.estate_get_inventory_sold_last_tick(v, item)
                    sold_cost = sold_cost + sold * DATA.province_get_local_prices(province, item)
                end)
                ut.money_entry("", DATA.estate_get_balance_last_tick(v), rect,
                    "Spend: " .. ut.to_fixed_point2(bought_cost) .. "\nEarn: " .. ut.to_fixed_point2(sold_cost)
                )
            end,
            width = base_unit * 3,
            ---@param v estate_id
            value = function(k, v)
                return DATA.estate_get_balance_last_tick(v)
            end
        },
        {
            header = "Workers",
            ---@param v estate_id
            render_closure = function(rect, k, v)
                local workers_needed = 0
                local workers_total = 0

                DATA.for_each_building_estate_from_estate(v, function (item)
                    local building = DATA.building_estate_get_building(item)
                    workers_total = workers_total + economy_values.amount_of_workers(building)
                    workers_needed = workers_needed + 1
                end)

                ut.data_entry(
                    "",
                    tostring(workers_total) .. "/" .. tostring(workers_needed),
                    rect
                )
            end,
            width = base_unit * 3,
            value = function(k, v)
                local workers_total = 0

                DATA.for_each_building_estate_from_estate(v, function (item)
                    workers_total = workers_total + 1
                end)

                return workers_total
            end
        },
    }

    ---@type estate_id[]
    local estates = {}

    local player = WORLD.player_character

    if player ~= INVALID_ID then
        estates = tabb.map_array(DATA.filter_array_ownership_from_owner(player, function (item)
            return true
        end), DATA.ownership_get_estate)
    end

    ut.table(rect, estates, columns, state)
end

return inspector