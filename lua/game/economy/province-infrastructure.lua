local pop_utils = require "game.entities.pop".POP

local r = {}

---
---@param province province_id
function r.run(province)
	---#logging LOGS:write("province infrastructure " .. tostring(province).."\n")
	---#logging LOGS:flush()

	-- First, calculate infrastructure needs
	local inf = 0
	local fat_province = DATA.fatten_province(province)

	-- From pops
	for _, pop_location in pairs(DATA.get_pop_location_from_location(province)) do
		local pop = DATA.pop_location_get_pop(pop_location)

		local race = DATA.fatten_race(DATA.pop_get_race(pop))
		local female = DATA.pop_get_female(pop)

		local n = race.male_infrastructure_needs
		if female then
			n = race.female_infrastructure_needs
		end
		---@type number
		inf = inf + n * AGE_MULTIPLIER(pop)
	end

	-- From buildings
	DATA.for_each_estate_location_from_province(province, function (estate_location)
		local estate = DATA.estate_location_get_estate(estate_location)
		DATA.for_each_building_estate_from_estate(estate, function (item)
			local building = DATA.building_estate_get_building(item)
			local building_type = DATA.building_get_current_type(building)
			local infrastructure_needs = DATA.building_type_get_needed_infrastructure(building_type)
			---@type number
			inf = inf + infrastructure_needs
		end)
	end)

	-- Write the needs
	fat_province.infrastructure_needed = inf

	-- Once we know the needed infrastructure, handle investments
	local inv = fat_province.infrastructure_investment
	local spillover = 0
	if inv > inf then
		spillover = inv - inf
	end
	-- If we're overinvested, remove a fraction above the invested amount
	inv = inv - spillover * 0.9

	-- Lastly, invest a fraction of the investment into actual infrastructure
	local invested = inv * (1 / (12 * 5)) -- 5 years to invest everything
	fat_province.infrastructure_investment = inv - invested
	fat_province.infrastructure = fat_province.infrastructure + invested

	-- At the very end, apply some decay to present infrastructure as to prevent runaway growth
	local infrastructure_decay_rate = 1 - 1 / (12 * 100) -- 100 years to decay everything
	if fat_province.infrastructure > inf then
		infrastructure_decay_rate = 1 - 1 / (12 * 50) -- 50 years to decay the part above the needed amount
	end
	fat_province.infrastructure = fat_province.infrastructure * infrastructure_decay_rate
end

return r
