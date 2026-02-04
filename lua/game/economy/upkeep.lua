local province_utils = require "game.entities.province".Province

local economic_effects = require "game.raws.effects.economy"
local upk = {}

---Runs upkeep on buildings in a province and destroys buildings if upkeep needs aren't met!
---@param province_id province_id
function upk.run(province_id)
	-- removing upkeep for now: unused mechanic
	-- replace with something more interesting later
end

return upk
