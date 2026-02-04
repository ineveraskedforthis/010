local character_values = require "game.raws.values.character"
local warband_utils = require "game.entities.warband"

local military_values = {}

---Returns scalar field representing how fast army can move in this tile
---@param army warband_id[]
---@return speed
function military_values.army_speed(army)
    -- speed is a minimal speed across all warbands
	---@type speed
	local result = {
		base = 9999,
		can_fly = true,
		forest_fast = true,
		river_fast = true
	}

	for _, warband in pairs(army) do
		local speed = military_values.warband_speed(warband)

		result.base = math.min(result.base, speed.base)
		result.can_fly = result.can_fly and speed.can_fly
		result.forest_fast = result.forest_fast and speed.forest_fast
		result.river_fast = result.river_fast and speed.river_fast
	end

	return result
end

---Returns scalar field representing how fast warband can move in this tile
---@param warband Warband
---@return speed
---@return number weight_mod
function military_values.warband_speed(warband)

	local total_hauling = warband_utils.total_hauling(warband)
	local total_weight = warband_utils.current_hauling(warband)

    -- speed is a minimal speed across all units
	---@type speed
	local result = {
		base = 9999,
		can_fly = true,
		forest_fast = true,
		river_fast = true
	}

	DATA.for_each_warband_unit_from_warband(warband, function (item)
		local pop = DATA.warband_unit_get_unit(item)
		local age = AGE_YEARS(pop)
		local child_age = DATA.race_get_child_age(RACE(pop))
--		if age < child_age then
--			-- babies are carried, add to total weight
--			total_weight = total_weight + require "game.entities.pop".POP.get_size(pop)
--		else
			local speed = character_values.travel_speed(pop)
			result.base = math.min(result.base, speed.base)
			result.can_fly = result.can_fly and speed.can_fly
			result.forest_fast = result.forest_fast and speed.forest_fast
			result.river_fast = result.river_fast and speed.river_fast
--		end
	end)

	-- from ~100% @ 0% to ~0% @ 200% weight:hauling
	-- 1/(1+math.exp(-3(0/_-1))) = 0.04742587317
	local weight_mod = math.min(1,1.048 - 1 / (1+math.exp(-3*(total_weight/total_hauling-1))))
	result.base = result.base * weight_mod

	return result, weight_mod
end

return military_values