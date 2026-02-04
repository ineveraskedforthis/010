require "codegen-lua.biome"

local Biome = {}
Biome.__index = Biome

---@class (exact) biome_id_data_blob_definition
---@field name string
---@field r number
---@field g number
---@field b number
---@field aquatic boolean?
---@field marsh boolean?
---@field icy boolean?
---@field minimum_slope number? m
---@field maximum_slope number? m
---@field minimum_elevation number? m
---@field maximum_elevation number? m
---@field minimum_temperature number? C
---@field maximum_temperature number? C
---@field minimum_summer_temperature number? C
---@field maximum_summer_temperature number? C
---@field minimum_winter_temperature number? C
---@field maximum_winter_temperature number? C
---@field minimum_rain number? mm
---@field maximum_rain number? mm
---@field minimum_available_water number? abstract, adjusted for permeability
---@field maximum_available_water number? abstract, adjusted for permeability
---@field minimum_trees number? %
---@field maximum_trees number? %
---@field minimum_grass number? %
---@field maximum_grass number? %
---@field minimum_shrubs number? %
---@field maximum_shrubs number? %
---@field minimum_conifer_fraction number? %
---@field maximum_conifer_fraction number? %
---@field minimum_dead_land number? %
---@field maximum_dead_land number? %
---@field minimum_soil_depth number? m
---@field maximum_soil_depth number? m
---@field minimum_soil_richness number? %
---@field maximum_soil_richness number? %
---@field minimum_sand number? %
---@field maximum_sand number? %
---@field minimum_clay number? %
---@field maximum_clay number? %
---@field minimum_silt number? %
---@field maximum_silt number? %

---@param data biome_id_data_blob_definition
---@return biome_id
function Biome:new(data)
	local id = BIOME.create()
	BIOME.set_aquatic(id, false)
	BIOME.set_marsh(id, false)
	BIOME.set_icy(id, false)
	BIOME.set_minimum_slope(id, -99999999)
	BIOME.set_maximum_slope(id, 99999999)
	BIOME.set_minimum_elevation(id, -99999999)
	BIOME.set_maximum_elevation(id, 99999999)
	BIOME.set_minimum_temperature(id, -99999999)
	BIOME.set_maximum_temperature(id, 99999999)
	BIOME.set_minimum_summer_temperature(id, -99999999)
	BIOME.set_maximum_summer_temperature(id, 99999999)
	BIOME.set_minimum_winter_temperature(id, -99999999)
	BIOME.set_maximum_winter_temperature(id, 99999999)
	BIOME.set_minimum_rain(id, -99999999)
	BIOME.set_maximum_rain(id, 99999999)
	BIOME.set_minimum_available_water(id, -99999999)
	BIOME.set_maximum_available_water(id, 99999999)
	BIOME.set_minimum_trees(id, -99999999)
	BIOME.set_maximum_trees(id, 99999999)
	BIOME.set_minimum_grass(id, -99999999)
	BIOME.set_maximum_grass(id, 99999999)
	BIOME.set_minimum_shrubs(id, -99999999)
	BIOME.set_maximum_shrubs(id, 99999999)
	BIOME.set_minimum_conifer_fraction(id, -99999999)
	BIOME.set_maximum_conifer_fraction(id, 99999999)
	BIOME.set_minimum_dead_land(id, -99999999)
	BIOME.set_maximum_dead_land(id, 99999999)
	BIOME.set_minimum_soil_depth(id, -99999999)
	BIOME.set_maximum_soil_depth(id, 99999999)
	BIOME.set_minimum_soil_richness(id, -99999999)
	BIOME.set_maximum_soil_richness(id, 99999999)
	BIOME.set_minimum_sand(id, -99999999)
	BIOME.set_maximum_sand(id, 99999999)
	BIOME.set_minimum_clay(id, -99999999)
	BIOME.set_maximum_clay(id, 99999999)
	BIOME.set_minimum_silt(id, -99999999)
	BIOME.set_maximum_silt(id, 99999999)
	BIOME.set_r(id, data.r)
	BIOME.set_g(id, data.g)
	BIOME.set_b(id, data.b)
	if data.aquatic ~= nil then
		BIOME.set_aquatic(id, data.aquatic)
	end
	if data.marsh ~= nil then
		BIOME.set_marsh(id, data.marsh)
	end
	if data.icy ~= nil then
		BIOME.set_icy(id, data.icy)
	end
	if data.minimum_slope ~= nil then
		BIOME.set_minimum_slope(id, data.minimum_slope)
	end
	if data.maximum_slope ~= nil then
		BIOME.set_maximum_slope(id, data.maximum_slope)
	end
	if data.minimum_elevation ~= nil then
		BIOME.set_minimum_elevation(id, data.minimum_elevation)
	end
	if data.maximum_elevation ~= nil then
		BIOME.set_maximum_elevation(id, data.maximum_elevation)
	end
	if data.minimum_temperature ~= nil then
		BIOME.set_minimum_temperature(id, data.minimum_temperature)
	end
	if data.maximum_temperature ~= nil then
		BIOME.set_maximum_temperature(id, data.maximum_temperature)
	end
	if data.minimum_summer_temperature ~= nil then
		BIOME.set_minimum_summer_temperature(id, data.minimum_summer_temperature)
	end
	if data.maximum_summer_temperature ~= nil then
		BIOME.set_maximum_summer_temperature(id, data.maximum_summer_temperature)
	end
	if data.minimum_winter_temperature ~= nil then
		BIOME.set_minimum_winter_temperature(id, data.minimum_winter_temperature)
	end
	if data.maximum_winter_temperature ~= nil then
		BIOME.set_maximum_winter_temperature(id, data.maximum_winter_temperature)
	end
	if data.minimum_rain ~= nil then
		BIOME.set_minimum_rain(id, data.minimum_rain)
	end
	if data.maximum_rain ~= nil then
		BIOME.set_maximum_rain(id, data.maximum_rain)
	end
	if data.minimum_available_water ~= nil then
		BIOME.set_minimum_available_water(id, data.minimum_available_water)
	end
	if data.maximum_available_water ~= nil then
		BIOME.set_maximum_available_water(id, data.maximum_available_water)
	end
	if data.minimum_trees ~= nil then
		BIOME.set_minimum_trees(id, data.minimum_trees)
	end
	if data.maximum_trees ~= nil then
		BIOME.set_maximum_trees(id, data.maximum_trees)
	end
	if data.minimum_grass ~= nil then
		BIOME.set_minimum_grass(id, data.minimum_grass)
	end
	if data.maximum_grass ~= nil then
		BIOME.set_maximum_grass(id, data.maximum_grass)
	end
	if data.minimum_shrubs ~= nil then
		BIOME.set_minimum_shrubs(id, data.minimum_shrubs)
	end
	if data.maximum_shrubs ~= nil then
		BIOME.set_maximum_shrubs(id, data.maximum_shrubs)
	end
	if data.minimum_conifer_fraction ~= nil then
		BIOME.set_minimum_conifer_fraction(id, data.minimum_conifer_fraction)
	end
	if data.maximum_conifer_fraction ~= nil then
		BIOME.set_maximum_conifer_fraction(id, data.maximum_conifer_fraction)
	end
	if data.minimum_dead_land ~= nil then
		BIOME.set_minimum_dead_land(id, data.minimum_dead_land)
	end
	if data.maximum_dead_land ~= nil then
		BIOME.set_maximum_dead_land(id, data.maximum_dead_land)
	end
	if data.minimum_soil_depth ~= nil then
		BIOME.set_minimum_soil_depth(id, data.minimum_soil_depth)
	end
	if data.maximum_soil_depth ~= nil then
		BIOME.set_maximum_soil_depth(id, data.maximum_soil_depth)
	end
	if data.minimum_soil_richness ~= nil then
		BIOME.set_minimum_soil_richness(id, data.minimum_soil_richness)
	end
	if data.maximum_soil_richness ~= nil then
		BIOME.set_maximum_soil_richness(id, data.maximum_soil_richness)
	end
	if data.minimum_sand ~= nil then
		BIOME.set_minimum_sand(id, data.minimum_sand)
	end
	if data.maximum_sand ~= nil then
		BIOME.set_maximum_sand(id, data.maximum_sand)
	end
	if data.minimum_clay ~= nil then
		BIOME.set_minimum_clay(id, data.minimum_clay)
	end
	if data.maximum_clay ~= nil then
		BIOME.set_maximum_clay(id, data.maximum_clay)
	end
	if data.minimum_silt ~= nil then
		BIOME.set_minimum_silt(id, data.minimum_silt)
	end
	if data.maximum_silt ~= nil then
		BIOME.set_maximum_silt(id, data.maximum_silt)
	end

	print(data.name)
	if RAWS_MANAGER.biomes_by_name[data.name] ~= nil then
		local msg = "Failed to load bedrock type (" .. tostring(data.name) .. ")"
		print(msg)
		error(msg)
	end
	RAWS_MANAGER.biomes_by_name[data.name] = id

	return id
end

return Biome
