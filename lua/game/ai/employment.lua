local economy_values = require "game.raws.values.economy"
local demography_values = require "game.raws.values.demography"
local demography_effects = require "game.raws.effects.demography"
local economy_effects = require "game.raws.effects.economy"

local tabb = require "engine.table"

---Handles ai looking for workers to work in their buildings
---@param province province_id
local function find_workers(province)
	PROFILER:start_timer("employment ai")
	local available_workers = demography_values.unemployed_pops(province)
	local current_worker = tabb.random_select_from_array(available_workers)
	if current_worker == nil then
		return
	end

	local cost = economy_values.pop_employment_cost(current_worker)

	--- for now let owners grab the worker from the pool
	local buildings = demography_values.vacant_buildings_owned_by_locally_present_pops(province)

	if #buildings == 0 then
		return
	end

	for index, value in ipairs(buildings) do
		--- ai owners express desire to employ the worker
		--- first one gets the worker
		local owner = OWNER(BUILDING_ESTATE(value))

		if not DATA.pop_get_is_player(owner) then
			local prediction = economy_values.projected_income(
				value,
				DATA.pop_get_race(current_worker),
				DATA.pop_get_female(current_worker)
			)

			if
				prediction > 1
				and prediction > DATA.pop_get_expected_wage(current_worker)
				and SAVINGS(owner) > cost
			then
				economy_effects.gift_to_pop(owner, current_worker, cost)
				demography_effects.employ_pop(current_worker, value)
				return
			end
		end
	end

	-- if pop failed all employment checks, his self-value decreases
	local current_value = DATA.pop_get_expected_wage(current_worker)
	DATA.pop_set_expected_wage(current_worker, math.max(0, current_value - 0.1))

	PROFILER:end_timer("employment ai")
end

return find_workers