local tabb = require "engine.table"

local values = {}

---@param province province_id
---@return pop_id[]
function values.unemployed_pops(province)
	return tabb.filter_array(
		tabb.map_array(DATA.get_pop_location_from_location(province), DATA.pop_location_get_pop),
		function (pop)
			local employment = DATA.get_employment_from_worker(pop)
			local building = DATA.employment_get_building(employment)
			local unit = DATA.get_warband_unit_from_unit(pop)
			local warband = DATA.warband_unit_get_warband(pop)
			local race = DATA.pop_get_race(pop)
			local teen_age = DATA.race_get_teen_age(race)
			return AGE_YEARS(pop) >= teen_age
				and (warband == INVALID_ID or UNIT_TYPE_OF(pop) == UNIT_TYPE.FOLLOWER)
				and building == INVALID_ID and not IS_CHARACTER(pop)
		end
	)
end

---@param province province_id
---@return building_id[]
function values.vacant_buildings_owned_by_locally_present_pops(province)
	local result = {}
	DATA.for_each_estate_location_from_province(province, function (estate_location)
		local estate = DATA.estate_location_get_estate(estate_location)
		local owner = OWNER(estate)
		if PROVINCE(owner) ~= province then
			return
		end
		DATA.for_each_building_estate_from_estate(estate, function (building_location)
			local building = DATA.building_estate_get_building(building_location)
			local employment = DATA.get_employment_from_building(building)
			local worker = DATA.employment_get_worker(employment)
			if worker == INVALID_ID then
				table.insert(result, building)
			end
		end)
	end)

	return result
end

---commenting
---@param province_id Province
---@return Character|nil
function values.sample_character_from_province(province_id)
	local characters = tabb.map_array(
		DATA.filter_array_character_location_from_location(province_id, ACCEPT_ALL),
		DATA.character_location_get_character
	)

	local amount = #characters
	if amount == 0 then
		return nil
	end

	local sample_index = love.math.random(amount)
	return characters[sample_index]
end


---all characters are pops
---@param province_id Province
---@return pop_id|nil
function values.sample_pop_from_province(province_id)
	local pops = tabb.map_array(
		DATA.filter_array_pop_location_from_location(province_id, ACCEPT_ALL),
		DATA.pop_location_get_pop
	)

	local amount = #pops
	if amount == 0 then
		return nil
	end

	local sample_index = love.math.random(amount)
	return pops[sample_index]
end

---all characters are pops
---@param province_id Province
---@return pop_id|nil
function values.sample_non_character_pop_from_province(province_id)
	local pops = tabb.map_array(
		DATA.filter_array_pop_location_from_location(province_id, function (item)
			local pop = DATA.pop_location_get_pop(item)
			if IS_CHARACTER(pop) then
				return false
			end
			return true
		end),
		DATA.pop_location_get_pop
	)

	local amount = #pops
	if amount == 0 then
		return nil
	end

	local sample_index = love.math.random(amount)
	return pops[sample_index]
end

---Returns a potential job, if a pop was to be employed by this building.
---@param building building_id
---@return job_id?
function values.potential_job(building)
	local btype = DATA.building_get_current_type(building)
	local method = DATA.building_type_get_production_method(btype)
	local job = DATA.production_method_get_job(method)
	local employment = DATA.get_employment_from_building(building)
	local worker = DATA.employment_get_worker(employment)
	if worker == INVALID_ID then
		return job
	end
	return nil
end

return values