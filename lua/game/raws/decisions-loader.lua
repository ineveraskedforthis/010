
local tabb = require "engine.table"
local ll = {}
local military_effects = require "game.raws.effects.military"
local utils = require "game.raws.raws-utils"
local province_utils = require "game.entities.province".Province
local economic_effects = require "game.raws.effects.economy"

function ll.load()
	local Decision = require "game.raws.decisions"

	require "game.raws.decisions._loader"()

	-- Logic flow:
	-- 1. Loop through all realms
	-- 2. Loop through all decisions
	-- 3. Check base probability (AI only) << base_probability >>
	-- 4. Check pretrigger << pretrigger >>
	-- 5. Select target (AI only) << ai_target >>
	-- 6. Check clickability << clickable >>
	-- 6a. If clickability failed, go back to 5, up to << ai_targetting_attempts >> times (AI only)
	-- 7. Select secondary target (AI only) << ai_secondary_target >>
	-- 8. Check is the decision is available (can be used on that specific target) << available >>
	-- 9. Check action probability (AI only) << ai_will_do >>
	-- 10. Apply decisions << effect >>

	--[[
	Decision.Realm:new {
		name = 'cheat-for-money',
		ui_name = 'Money Cheat',
		tooltip = "Because developers don't wanna wait for monthly income when testing buildings",
		sorting = 0,
		base_probability = 0,
		effect = function(realm, primary_target, secondary_target)
			realm.treasury = realm.treasury + 1000
		end,
	}
	Decision.Realm:new {
		name = 'never-possible',
		ui_name = 'this should never be visible',
		sorting = 0,
		secondary_target = 'tile',
		base_probability = 0,
		effect = function(realm, primary_target, secondary_target)
			print("This should never happen!")
		end,
		pretrigger = function()
			return false
		end
	}
	Decision.Realm:new {
		name = 'target-debug',
		ui_name = 'debugging (province selection)',
		tooltip = "This decision does nothing. It exists only to debug secondary target selection",
		sorting = 0,
		primary_target = 'province',
		secondary_target = 'province',
		base_probability = 0, -- AI will never do this, it's just for debugging the system
		effect = function(realm, primary_target, secondary_target)
			print("Stuff is happening!")
			WORLD:emit_event(RAWS_MANAGER.events_by_name['default'], realm, nil)
		end,
		clickable = function(realm, primary_target)
			return primary_target.realm == realm
		end,
		get_secondary_targets = function(realm, primary_target)
			local r = {}
			for _, province in pairs(realm.provinces) do
				r[#r + 1] = province
			end
			return r
		end,
	}
	--]]
end

return ll
