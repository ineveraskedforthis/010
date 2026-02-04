local tabb = require "engine.table"
local bld = {}

local economy_values = require "game.raws.values.economy"
local economy_effects = require "game.raws.effects.economy"

local method_utils = require "game.raws.production-methods"

---@class (exact) CandidateBuilding
---@field profit number
---@field cost number
---@field type building_type_id

---Employs pops in the province.
---@param province province_id
function bld.run(province)
    -- maybe run some events later?
	-- now buildings do not clutter UI, so no need to delete them
end

return bld