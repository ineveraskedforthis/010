local pop_utils = require "game.entities.pop".POP
local warband_utils = require "game.entities.warband"
local warband_effects = require "game.raws.effects.warband"

local army_utils = {}

---@param army warband_id[]
---@return number
function army_utils.get_visibility(army)
	local vis = 0
	for _, warband in pairs(army) do
		vis = vis + warband_utils.visibility(warband)
	end
	return vis
end

---@param army warband_id[]
---@return number
function army_utils.size(army)
	local result = 0
	for _, warband in pairs(army) do
		result = result + warband_utils.size(warband)
	end
	return result
end

---@param army warband_id[]
---@return number
function army_utils.loot_capacity(army)
	local cap = 0
	for _, warband in pairs(army) do
		cap = cap + warband_utils.loot_capacity(warband)
	end
	return cap
end

---Kill everyone in the army
---@param army warband_id[]
function army_utils.decimate(army)
	for _, warband in pairs(army) do
		warband_effects.decimate(warband)
	end
end

---Returns the pop membership in the army
---@param army warband_id[]
---@return warband_unit_id[]
function army_utils.pops(army)
	local res = {}
	for _, warband in pairs(army) do
		for _, unit in pairs(DATA.get_warband_unit_from_warband(warband)) do
			table.insert(res, unit)
		end
	end
	return res
end

return army_utils
