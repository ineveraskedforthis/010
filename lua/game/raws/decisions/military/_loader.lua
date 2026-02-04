local tabb = require "engine.table"
local path = require "game.ai.pathfinding"

local utils = require "game.raws.raws-utils"
local Decision = require "game.raws.decisions"
local tooltiped_triggers = require "game.raws.triggers.tooltiped_triggers"

local realm_utils = require "game.entities.realm".Realm
local province_utils = require "game.entities.province".Province

local office_triggers = require "game.raws.triggers.offices"
local diplomacy_trigggers = require "game.raws.triggers.diplomacy"
local quests_triggers = require "game.raws.triggers.quests"

local military_values = require "game.raws.values.military"
local diplomacy_values = require "game.raws.values.diplomacy"
local ai_values = require "game.raws.values.ai"

local military_effects = require "game.raws.effects.military"
local economic_effects = require "game.raws.effects.economy"




local NOT_BUSY = tooltiped_triggers.Pretrigger.not_busy
local OR = tooltiped_triggers.Pretrigger.OR
local LEADING_WARBAND_IDLE = tooltiped_triggers.Pretrigger.leading_idle_warband
local LEADINING_GUARD_IDLE = tooltiped_triggers.Pretrigger.leading_idle_guard

return function ()
	local base_raiding_reward = 20
	---@type DecisionCharacterProvince
	Decision.CharacterProvince:new {
		name = 'invest-quest-raid',
		ui_name = "Provide raiding quest reward " .. tostring(base_raiding_reward),
		tooltip = utils.constant_string("Declare province as target for future raids. Can avoid diplomatic issues. Loots only from the local provincial wealth pool."),
		sorting = 1,
		primary_target = "province",
		secondary_target = 'none',
		base_probability = 1 / 12 / 4,
		path = nil,
		pretrigger = function(root)
			if BUSY(root) then return false end
			if SAVINGS(root) < base_raiding_reward or realm_utils.get_realm_ready_military(LOCAL_REALM(root)) == 0 then
				return false
			end
			return true
		end,
		clickable = function(root, primary_target)
			if PROVINCE_REALM(primary_target) == LOCAL_REALM(root) then
				return false
			end
			return province_utils.neighbors_realm(primary_target, LOCAL_REALM(root))
		end,
		available = function(root, primary_target)
			if PROVINCE_REALM(primary_target) == INVALID_ID then
				return false
			end
			if diplomacy_trigggers.pays_tribute_to(PROVINCE_REALM(primary_target), LOCAL_REALM(root)) then
				return false
			end
			return true
		end,
		ai_will_do = function(root, primary_target, secondary_target)
			--print("aiw")
			return SAVINGS(root) / base_raiding_reward / 100 -- 1% chance when have enough wealth
		end,
		ai_targetting_attempts = 2,
		ai_target = function(root)
			for _, province in pairs(DATA.realm_get_known_provinces(REALM(root))) do
				if PROVINCE_REALM(province) == INVALID_ID then
					goto continue
				end

				if realm_utils.is_realm_in_hierarchy(PROVINCE_REALM(province), LOCAL_REALM(root)) then
					goto continue
				end

				if realm_utils.is_realm_in_hierarchy(LOCAL_REALM(root), PROVINCE_REALM(province)) then
					goto continue
				end

				do
					return province, true
				end

				::continue::
			end
			return nil, false
		end,
		ai_secondary_target = function(root, primary_target)
			--print("ais")
			return nil, true
		end,
		effect = function(root, primary_target, secondary_target)
			local realm = REALM(root)
			assert(realm ~= INVALID_ID, "INVALID REALM")

			if DATA.realm_get_quests_raid(realm)[primary_target] == nil then
				DATA.realm_get_quests_raid(realm)[primary_target] = 0
			end

			DATA.realm_get_quests_raid(realm)[primary_target] = DATA.realm_get_quests_raid(realm)[primary_target] + base_raiding_reward
			economic_effects.add_pop_savings(root, -base_raiding_reward, ECONOMY_REASON.QUEST)
		end
	}

	---@type DecisionCharacterProvince
	Decision.CharacterProvince:new {
		name = 'invest-quest-explore',
		ui_name = "Provide exploration quest reward " .. tostring(base_raiding_reward),
		tooltip = utils.constant_string("Declare province as exploration target"),
		sorting = 1,
		primary_target = "province",
		secondary_target = 'none',
		base_probability = 1 / 12 / 4,
		path = nil,
		pretrigger = function(root)
			if BUSY(root) then return false end
			if SAVINGS(root) < base_raiding_reward then
				return false
			end
			return true
		end,
		clickable = function(root, primary_target)
			return quests_triggers.eligible_for_exploration(REALM(root), primary_target)
		end,
		available = function(root, primary_target)
			return true
		end,
		ai_will_do = function(root, primary_target, secondary_target)
			--print("aiw")
			return SAVINGS(root) / base_raiding_reward / 100 -- 1% chance when have enough wealth
		end,
		ai_targetting_attempts = 2,
		ai_target = function(root)
			for _, province in pairs(DATA.realm_get_known_provinces(REALM(root))) do
				if quests_triggers.eligible_for_exploration(REALM(root), province) and love.math.random() < 0.1 then
					return province, true
				end
			end
			return nil, false
		end,
		ai_secondary_target = function(root, primary_target)
			--print("ais")
			return nil, true
		end,
		effect = function(root, primary_target, secondary_target)
			local realm = REALM(root)
			assert(realm ~= INVALID_ID, "INVALID REALM")

			if DATA.realm_get_quests_explore(realm)[primary_target] == nil then
				DATA.realm_get_quests_explore(realm)[primary_target] = 0
			end
			DATA.realm_get_quests_explore(realm)[primary_target] = DATA.realm_get_quests_explore(realm)[primary_target] + base_raiding_reward
			economic_effects.add_pop_savings(root, -base_raiding_reward, ECONOMY_REASON.QUEST)
		end
	}

	---@type DecisionCharacterProvince
	Decision.CharacterProvince:new {
		name = 'invest-quest-patrol',
		ui_name = "Provide patrol quest reward " .. tostring(base_raiding_reward),
		tooltip = utils.constant_string("Declare province as patrol target"),
		sorting = 1,
		primary_target = "province",
		secondary_target = 'none',
		base_probability = 1 / 12 / 4,
		path = nil,
		pretrigger = function(root)
			if BUSY(root) then return false end
			if SAVINGS(root) < base_raiding_reward then
				return false
			end
			return true
		end,
		clickable = function(root, primary_target)
			if realm_utils.is_realm_in_hierarchy(REALM(root), PROVINCE_REALM(primary_target)) then
				return true
			end
			return false
		end,
		available = function(root, primary_target)
			return true
		end,
		ai_will_do = function(root, primary_target, secondary_target)
			--print("aiw")
			return SAVINGS(root) / base_raiding_reward / 100 -- 1% chance when have enough wealth
		end,
		ai_targetting_attempts = 2,
		ai_target = function(root)
			local result = diplomacy_values.sample_tributary(REALM(root))
			if result ~= nil and love.math.random() < 0.2 then
				return CAPITOL(result), true
			end
			return CAPITOL(REALM(root)), true
		end,
		ai_secondary_target = function(root, primary_target)
			--print("ais")
			return nil, true
		end,
		effect = function(root, primary_target, secondary_target)
			local realm = REALM(root)
			assert(realm ~= nil, "INVALID REALM")

			if DATA.realm_get_quests_patrol(realm)[primary_target] == nil then
				DATA.realm_get_quests_patrol(realm)[primary_target] = 0
			end

			DATA.realm_get_quests_patrol(realm)[primary_target] = DATA.realm_get_quests_patrol(realm)[primary_target] + base_raiding_reward
			economic_effects.add_pop_savings(root, -base_raiding_reward, ECONOMY_REASON.QUEST)
		end
	}
end