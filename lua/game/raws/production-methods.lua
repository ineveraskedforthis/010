require "codegen-lua.activity"
local dbm = require "game.economy.diet-breadth-model"
local tile_utils = require "game.entities.tile"

local ProductionMethod = {}

---@class (exact) activity_id_data_blob_definition
---@field name string
---@field icon string
---@field description string
---@field r number
---@field g number
---@field b number
---@field job_type JOBTYPE
---@field job job_id
---@field foraging boolean? If true, worktime counts towards the foragers count
---@field hydration boolean? If true, worktime counts towards the foragers_water count
---@field nature_yield_dependence number? How much does the local flora and fauna impact this buildings yield? Defaults to 0
---@field forest_dependence number? Set to 1 if building consumes local forests
---@field crop boolean? If true, the building will periodically change its yield for a season.
---@field temperature_ideal_min number?
---@field temperature_ideal_max number?
---@field temperature_extreme_min number?
---@field temperature_extreme_max number?
---@field rainfall_ideal_min number?
---@field rainfall_ideal_max number?
---@field rainfall_extreme_min number?
---@field rainfall_extreme_max number?
---@field clay_ideal_min number?
---@field clay_ideal_max number?
---@field clay_extreme_min number?
---@field clay_extreme_max number?

---@class activity_id_data_blob_definition_extended : activity_id_data_blob_definition
---@field inputs table<use_case_id, number>
---@field outputs table<trade_good_id, number>
---@field job job_id

---Creates a new production method
---@param data activity_id_data_blob_definition_extended
---@return activity_id
function ProductionMethod:new(data)
	if RAWS_MANAGER.do_logging then
		print("ProductionMethod: " .. data.name)
	end

	local id = ACTIVITY.create()
	ACTIVITY.set_foraging(id, false)
	ACTIVITY.set_hydration(id, false)
	ACTIVITY.set_nature_yield_dependence(id, 0)
	ACTIVITY.set_forest_dependence(id, 0)
	ACTIVITY.set_crop(id, false)
	ACTIVITY.set_temperature_ideal_min(id, 10)
	ACTIVITY.set_temperature_ideal_max(id, 30)
	ACTIVITY.set_temperature_extreme_min(id, 0)
	ACTIVITY.set_temperature_extreme_max(id, 50)
	ACTIVITY.set_rainfall_ideal_min(id, 50)
	ACTIVITY.set_rainfall_ideal_max(id, 100)
	ACTIVITY.set_rainfall_extreme_min(id, 5)
	ACTIVITY.set_rainfall_extreme_max(id, 350)
	ACTIVITY.set_clay_ideal_min(id, 0)
	ACTIVITY.set_clay_ideal_max(id, 1)
	ACTIVITY.set_clay_extreme_min(id, 0)
	ACTIVITY.set_clay_extreme_max(id, 1)
	ACTIVITY.set_r(id, data.r)
	ACTIVITY.set_g(id, data.g)
	ACTIVITY.set_b(id, data.b)
	ACTIVITY.set_job_type(id, data.job_type)
	ACTIVITY.set_job(id, data.job)
	if data.foraging ~= nil then
		ACTIVITY.set_foraging(id, data.foraging)
	end
	if data.hydration ~= nil then
		ACTIVITY.set_hydration(id, data.hydration)
	end
	if data.nature_yield_dependence ~= nil then
		ACTIVITY.set_nature_yield_dependence(id, data.nature_yield_dependence)
	end
	if data.forest_dependence ~= nil then
		ACTIVITY.set_forest_dependence(id, data.forest_dependence)
	end
	if data.crop ~= nil then
		ACTIVITY.set_crop(id, data.crop)
	end
	if data.temperature_ideal_min ~= nil then
		ACTIVITY.set_temperature_ideal_min(id, data.temperature_ideal_min)
	end
	if data.temperature_ideal_max ~= nil then
		ACTIVITY.set_temperature_ideal_max(id, data.temperature_ideal_max)
	end
	if data.temperature_extreme_min ~= nil then
		ACTIVITY.set_temperature_extreme_min(id, data.temperature_extreme_min)
	end
	if data.temperature_extreme_max ~= nil then
		ACTIVITY.set_temperature_extreme_max(id, data.temperature_extreme_max)
	end
	if data.rainfall_ideal_min ~= nil then
		ACTIVITY.set_rainfall_ideal_min(id, data.rainfall_ideal_min)
	end
	if data.rainfall_ideal_max ~= nil then
		ACTIVITY.set_rainfall_ideal_max(id, data.rainfall_ideal_max)
	end
	if data.rainfall_extreme_min ~= nil then
		ACTIVITY.set_rainfall_extreme_min(id, data.rainfall_extreme_min)
	end
	if data.rainfall_extreme_max ~= nil then
		ACTIVITY.set_rainfall_extreme_max(id, data.rainfall_extreme_max)
	end
	if data.clay_ideal_min ~= nil then
		ACTIVITY.set_clay_ideal_min(id, data.clay_ideal_min)
	end
	if data.clay_ideal_max ~= nil then
		ACTIVITY.set_clay_ideal_max(id, data.clay_ideal_max)
	end
	if data.clay_extreme_min ~= nil then
		ACTIVITY.set_clay_extreme_min(id, data.clay_extreme_min)
	end
	if data.clay_extreme_max ~= nil then
		ACTIVITY.set_clay_extreme_max(id, data.clay_extreme_max)
	end

	local input_index = 1
	for use_case, amount in pairs(data.inputs) do
		ACTIVITY.set_inputs_amount(id, input_index, amount)
		ACTIVITY.set_inputs(id, input_index, use_case)
		input_index = input_index + 1
	end

	local output_index = 1
	for good, amount in pairs(data.outputs) do
		ACTIVITY.set_outputs_amount(id, output_index, amount)
		ACTIVITY.set_outputs(id, output_index, good)
		output_index = output_index + 1
	end

	if RAWS_MANAGER.production_methods_by_name[data.name] ~= nil then
		local msg = "Failed to load a production method (" .. tostring(data.name) .. ")"
		print(msg)
		error(msg)
	end
	RAWS_MANAGER.production_methods_by_name[data.name] = id
	return id
end

---@param method activity_id
---@param province province_id
---@return number
function ProductionMethod.get_efficiency(method, province)
	local fat_method = DATA.fatten_activity(method)
	local fat_province = DATA.fatten_province(province)

	-- Return 0 efficiency for water provinces
	if not TILE.get_is_land(fat_province.center) then
		return 0
	end

	local total_efficiency = 0
	DATA.for_each_tile_province_membership_from_province(province, function (item)
		local tile_id = TILE.province_membership_get_tile(item)
		local crop_yield = 1
		if fat_method.crop then
			local jan_rain, jan_temp, jul_rain, jul_temp = tile_utils.get_climate_data(tile_id)
			local t = (jan_temp + jul_temp) / 2
			local r = (jan_rain + jul_rain) / 2
			if r > fat_method.rainfall_ideal_min and r < fat_method.rainfall_ideal_max then
				-- Ideal conditions for growing this plant!
			elseif r < fat_method.rainfall_ideal_min then
				local d = (r - fat_method.rainfall_extreme_min) / (fat_method.rainfall_ideal_min - fat_method.rainfall_extreme_min)
				crop_yield = crop_yield * math.max(0, d)
			elseif r > fat_method.rainfall_ideal_max then
				local d = (r - fat_method.rainfall_ideal_max) /
					(fat_method.rainfall_extreme_max - fat_method.rainfall_ideal_max)
				d = 1 - d
				crop_yield = crop_yield * math.max(0, d)
			end
			if t > fat_method.temperature_ideal_min and r < fat_method.temperature_ideal_max then
				-- Ideal conditions for growing this plant!
			elseif t < fat_method.temperature_ideal_min then
				local d = (t - fat_method.temperature_extreme_min) /
					(fat_method.temperature_ideal_min - fat_method.temperature_extreme_min)
				crop_yield = crop_yield * math.max(0, d)
			elseif t > fat_method.temperature_ideal_max then
				local d = (t - fat_method.temperature_ideal_max) /
					(fat_method.temperature_extreme_max - fat_method.temperature_ideal_max)
				d = 1 - d
				crop_yield = crop_yield * math.max(0, d)
			end
		end
		local soil_efficiency = 1
		if fat_method.clay_ideal_min > 0 or fat_method.clay_ideal_max < 1 then
			local clay = TILE.get_clay(tile_id)
			if clay > fat_method.clay_ideal_min and clay < fat_method.clay_ideal_max then
				-- Ideal conditions!
			elseif clay < fat_method.clay_ideal_min then
				local d = (clay - fat_method.clay_extreme_min) / (fat_method.clay_ideal_min - fat_method.clay_extreme_min)
				soil_efficiency = soil_efficiency * math.max(0, d)
			elseif clay > fat_method.clay_ideal_max then
				local d = (clay - fat_method.clay_ideal_max) /
					(fat_method.clay_extreme_max - fat_method.clay_ideal_max)
				d = 1 - d
				soil_efficiency = soil_efficiency * math.max(0, d)
			end
		end
		total_efficiency = total_efficiency + crop_yield * soil_efficiency
	end)
	local nature_yield = 1
	if fat_method.foraging then
		nature_yield = nature_yield * dbm.foraging_efficiency(fat_province.foragers_limit, fat_province.foragers)
	end
	if fat_method.hydration then
		nature_yield = nature_yield * dbm.foraging_efficiency(fat_province.hydration, fat_province.foragers_water)
	end
	if fat_method.forest_dependence > 0 then
		local amount_of_wood = DATA.province_get_foragers_targets_amount(province, FORAGE_RESOURCE.WOOD)
		nature_yield = nature_yield * (amount_of_wood / fat_province.size) * fat_method.forest_dependence
	end
	if fat_method.nature_yield_dependence > 0 then
		nature_yield = nature_yield * math.max(0, fat_province.foragers_limit / fat_province.size) * fat_method.nature_yield_dependence
	end
	return total_efficiency * nature_yield / fat_province.size
end

return ProductionMethod
