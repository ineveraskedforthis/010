local tabb = require "engine.table"
local pop_utils = require "game.entities.pop".POP

local warband_utils = {}

-- values

---Returns a the highest ranking officer
---@param warband warband_id
---@return Character officer
function warband_utils.active_leader(warband)
	local leader = DATA.warband_leader_get_leader(DATA.get_warband_leader_from_warband(warband))
	if leader ~= INVALID_ID then
		return leader
	end
	local recruiter = DATA.warband_recruiter_get_recruiter(DATA.get_warband_recruiter_from_warband(warband))
	if recruiter ~= INVALID_ID then
		return recruiter
	end
	local commander = DATA.warband_commander_get_commander(DATA.get_warband_commander_from_warband(warband))
	if commander ~= INVALID_ID then
		return commander
	end
	return INVALID_ID
end

---Returns a the lowest ranking officer
---@param warband warband_id
---@return Character officer
function warband_utils.active_commander(warband)
	local commander = DATA.warband_commander_get_commander(DATA.get_warband_commander_from_warband(warband))
	if commander ~= INVALID_ID then
		return commander
	end
	local recruiter = DATA.warband_recruiter_get_recruiter(DATA.get_warband_recruiter_from_warband(warband))
	if recruiter ~= INVALID_ID then
		return recruiter
	end
	local leader = DATA.warband_leader_get_leader(DATA.get_warband_leader_from_warband(warband))
	if leader ~= INVALID_ID then
		return leader
	end
	return INVALID_ID
end

---Returns location of warband, either the leader's province or the guard realm
---@param warband warband_id
---@return tile_id
function warband_utils.location(warband)
	local location = DATA.get_warband_location_from_warband(warband)
	return DATA.warband_location_get_location(location)
end

---Returns realm of warband, either the leader's realm or the realm it's a guard of
---@param warband warband_id
---@return Realm
function warband_utils.realm(warband)
	local leader = DATA.warband_leader_get_leader(DATA.get_warband_leader_from_warband(warband))
	if leader ~= INVALID_ID then
		return REALM(leader)
	else
		local guard_of = DATA.realm_guard_get_realm(DATA.get_realm_guard_from_guard(warband))
		return guard_of
	end
end

---@param warband warband_id
---@return number
function warband_utils.loot_capacity(warband)
	return warband_utils.total_hauling(warband)
end

---@param warband warband_id
---@return number
function warband_utils.total_hauling(warband)
	local cap = 0
	DATA.for_each_warband_unit_from_warband(warband, function (item)
		local pop = DATA.warband_unit_get_unit(item)
		---@type number
		cap = cap + pop_utils.get_supply_capacity(pop)
	end)
	return cap
end

---@param warband warband_id
---@return number
function warband_utils.current_hauling(warband)
	local total_weight = 0
	DATA.for_each_trade_good(function (item)
		-- TODO: implement weight of trade goods
		total_weight = total_weight + DATA.warband_get_inventory(warband, item)
	end)
	return total_weight
end

---Returns warbands current spotting bonus
---@param warband warband_id
---@return number
function warband_utils.spotting(warband)
	---@type number
	local result = 0

	for _, membership in ipairs(DATA.get_warband_unit_from_warband(warband)) do
		local pop = DATA.warband_unit_get_unit(membership)
		---@type number
		result = result + pop_utils.get_spotting(pop)
	end
	local status = DATA.warband_get_current_status(warband)
	-- patrolling increases spotting
	if status == WARBAND_STATUS.PREPARING_PATROL
		or status == WARBAND_STATUS.PATROL then
		result = result * 10
	-- some pop time is always used by warband unless off duty
	elseif status ~= WARBAND_STATUS.OFF_DUTY then
		result = result * 5
	end
	return result
end

---Returns warbands current visibility
---@param warband warband_id
---@return number
function warband_utils.visibility(warband)
	---@type number
	local result = 0
	for _, membership in ipairs(DATA.get_warband_unit_from_warband(warband)) do
		local pop = DATA.warband_unit_get_unit(membership)
		---@type number
		result = result + pop_utils.get_visibility(pop)
	end

	return result
end

---Returns the fighting sum of all units health, attack, armor, and speed along with count,
--- optionally include civilians as combatants
---@param warband warband_id
---@param civilian boolean?
---@return number total_health
---@return number total_attack
---@return number total_armor
---@return number total_speed
---@return number total_count
function warband_utils.total_strength(warband, civilian)
	local total_health, total_attack, total_armor,total_speed, total_count = 0, 0, 0, 0 ,0
	for _, membership in ipairs(DATA.get_warband_unit_from_warband(warband)) do
		local pop = DATA.warband_unit_get_unit(membership)
		local unit_type = DATA.warband_unit_get_type(membership)
		if civilian or unit_type == UNIT_TYPE.WARRIOR then
			local health, attack, armor, speed = pop_utils.get_strength(pop)
			total_health = total_health + health
			total_attack = total_attack + attack
			total_armor = total_armor + armor
			total_speed = total_speed + speed
			total_count = total_count + 1
		end
	end
	return total_health, total_attack, total_armor, total_speed, total_count
end

---Total size of warband
---@param warband warband_id
---@return integer
function warband_utils.size(warband)
	local result = tabb.size(DATA.get_warband_unit_from_warband(warband))
	return result
end

---Target size of warband
---@param warband warband_id
---@return integer
function warband_utils.target_size(warband)
	local result = 0
	DATA.for_each_unit_type(function (item)
		result = result + DATA.warband_get_units_target(warband, item)
	end)

	return result
end

---Return the number of combat units
---@param warband warband_id
---@return integer
function warband_utils.war_size(warband)
	return tabb.size(DATA.filter_warband_unit_from_warband(warband, function(item)
		local unit_type = DATA.warband_unit_get_type(item)
		return unit_type == UNIT_TYPE.WARRIOR
	end))
end

---Predicts upkeep given the current units target of warbands
---@param warband warband_id
---@return number
function warband_utils.predict_upkeep(warband)
	local result = 0
	for _, membership in ipairs(DATA.get_warband_unit_from_warband(warband)) do
		local unit_type = DATA.warband_unit_get_type(membership)
		result = result + DATA.unit_type_get_base_cost(unit_type)
	end
	return result
end

---Returns monthly budget
---@param warband warband_id
---@return number
function warband_utils.monthly_budget(warband)
	return DATA.warband_get_treasury(warband) / 12
end

---Returs daily consumption of supplies.
---@param warband warband_id
---@return number
function warband_utils.daily_supply_consumption(warband)
	local result = 0
	DATA.for_each_warband_unit_from_warband(warband, function(item)
		local pop = DATA.warband_unit_get_unit(item)
		---@type number
		result = result + pop_utils.get_supply_use(pop)
	end)

	return result * 0.25 --- made up value. raw value leads to VERY expensive trading
end

---@param warband warband_id
function warband_utils.supplies_target(warband)
	return warband_utils.daily_supply_consumption(warband) * DATA.warband_get_supplies_target_days(warband)
end

---Returns speed of exploration
---@param warband warband_id
---@return number
function warband_utils.exploration_speed(warband)
	return warband_utils.size(warband) * (1 - DATA.warband_get_current_time_used_ratio(warband))
end

return warband_utils
