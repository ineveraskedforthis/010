local emp = {}
local demography_effects = require "game.raws.effects.demography"

---Update pops employment in the province.
---@param province province_id
function emp.run(province)
	--- check if some contracts are expired
	---@type pop_id[]
	local fire_list = {}
	DATA.for_each_estate_location_from_province(province, function (location)
		local estate = DATA.estate_location_get_estate(location)
		DATA.for_each_building_estate_from_estate(estate, function (item)
			local building = DATA.building_estate_get_building(item)
			local employment = DATA.get_employment_from_building(building)
			local start = DATA.employment_get_start_date(employment)
			local now = WORLD.day + WORLD.month * 30 + WORLD.year * 12 * 30
			if now - start > 12 * 30 * EMPLOYMENT_YEARS then
				table.insert(fire_list, DATA.employment_get_worker(employment))
			end
		end)
	end)

	for _, value in ipairs(fire_list) do
		demography_effects.fire_pop(value)
	end
end

return emp
