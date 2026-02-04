
local WarbandEffects = {}

---Sets character as a recruiter of warband
---@param character Character
---@param warband Warband
function WarbandEffects.set_recruiter(warband, character)
	local recruiter_warband = DATA.get_warband_recruiter_from_warband(warband)
	if recruiter_warband ~= INVALID_ID then
		DATA.warband_recruiter_set_recruiter(recruiter_warband, character)
	else
		DATA.force_create_warband_recruiter(character, warband)
	end
end

---unets character as a recruiter of warband
---@param warband Warband
function WarbandEffects.unset_recruiter(warband)
	local recruiter_warband = DATA.get_warband_recruiter_from_warband(warband)
	if recruiter_warband ~= INVALID_ID then
		DATA.delete_warband_recruiter(recruiter_warband)
	end
end

---Sets character as a recruiter of warband
---@param character Character
---@param warband Warband
function WarbandEffects.set_commander(warband, character)
	local commander_warband = DATA.get_warband_commander_from_warband(warband)
	if commander_warband ~= INVALID_ID then
		DATA.warband_commander_set_commander(commander_warband, character)
	else
		DATA.force_create_warband_commander(character, warband)
	end
end

---unets character as a recruiter of warband
---@param warband Warband
function WarbandEffects.unset_commander(warband)
	local commander_warband = DATA.get_warband_recruiter_from_warband(warband)
	if commander_warband ~= INVALID_ID then
		DATA.delete_warband_commander(commander_warband)
	end
end

---@param warband warband_id
---@param character Character
---@param unit UNIT_TYPE
function WarbandEffects.set_as_unit(warband, character, unit)
	---#logging LOGS:write("set character as a unit \n")
	---#logging LOGS:flush()
	local current_unit = DATA.get_warband_unit_from_unit(character)
	local current_type = DATA.warband_unit_get_type(current_unit)
	local current_warband = DATA.warband_unit_get_warband(current_unit)

	local fat_warband = DATA.fatten_warband(warband)

	local new_upkeep = DATA.unit_type_get_base_cost(unit)

	if current_warband == INVALID_ID then
		---#logging LOGS:write("no current warband\n")
		---#logging LOGS:flush()

		local new_membership = DATA.fatten_warband_unit(DATA.force_create_warband_unit(character, warband))
		new_membership.type = unit
	elseif current_warband ~= warband then
		---#logging LOGS:write("there is current warband but it's different\n")
		---#logging LOGS:flush()

		local current_upkeep = DATA.unit_type_get_base_cost(current_type)

		DATA.warband_inc_units_current(current_warband, current_type, -1)
		DATA.warband_inc_total_upkeep(current_warband, -current_upkeep)

		DATA.warband_unit_set_warband(current_unit, warband)
		DATA.warband_unit_set_type(current_unit, unit)
	else
		---#logging LOGS:write("there is current warband and it's the same\n")
		---#logging LOGS:flush()

		local current_upkeep = DATA.unit_type_get_base_cost(current_type)

		DATA.warband_inc_units_current(current_warband, current_type, -1)
		DATA.warband_inc_total_upkeep(current_warband, -current_upkeep)

		DATA.warband_unit_set_type(current_unit, unit)
	end

	fat_warband.total_upkeep = fat_warband.total_upkeep + new_upkeep
	DATA.warband_inc_units_current(warband, unit, 1)

	---#logging LOGS:write("taking up command was successful\n")
	---#logging LOGS:flush()
end


---Handles pop firing logic on warband's side
---@param warband warband_id
---@param pop pop_id
function WarbandEffects.fire_unit(warband, pop)
	-- print(pop.name, "leaves warband")
	local membership = DATA.get_warband_unit_from_unit(pop)
	local fat_membership = DATA.fatten_warband_unit(membership)

	assert(warband == fat_membership.warband, "INVALID OPERATION: POP WAS IN A WRONG WARBAND")

	DATA.warband_inc_units_current(warband, fat_membership.type, -1)
	DATA.warband_inc_total_upkeep(warband, -DATA.unit_type_get_base_cost(fat_membership.type))

	local recruit = DATA.get_warband_recruiter_from_warband(warband)
	if recruit ~= INVALID_ID then
		local recruiter = DATA.warband_recruiter_get_recruiter(recruit)
		if pop == recruiter then
			WarbandEffects.unset_recruiter(warband)
		end
	end
	local command = DATA.get_warband_commander_from_warband(warband)
	if command ~= INVALID_ID then
		local commander = DATA.warband_commander_get_commander(command)
		if pop == commander then
			WarbandEffects.unset_commander(warband)
		end
	end

	DATA.delete_warband_unit(membership)
end

---@param warband warband_id
function WarbandEffects.decimate(warband)
	local pops_to_delete = require "engine.table".map_array(DATA.get_warband_unit_from_warband(warband), DATA.warband_unit_get_unit)
	for _, pop in ipairs(pops_to_delete) do
		DATA.delete_pop(pop)
	end
end

return WarbandEffects