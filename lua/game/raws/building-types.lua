require "codegen-lua.building_type"
require "codegen-lua.technology_building"

local tabb = require "engine.table"

---@class (exact) building_type_id_data_blob_definition
---@field name string
---@field icon string
---@field description string
---@field r number
---@field g number
---@field b number
---@field construction_cost number
---@field movable boolean? is it possible to migrate with this building?
---@field archetype number

BuildingType = {}
BuildingType.__index = BuildingType
---Creates a new building
---@param data building_type_id_data_blob_definition
---@return building_type_id
function BuildingType:new(data)
	if RAWS_MANAGER.do_logging then
		print("BuildingType: " .. data.name)
	end

	local id = BUILDING_TYPE.create()
	BUILDING_TYPE.set_r(id, data.r)
	BUILDING_TYPE.set_g(id, data.g)
	BUILDING_TYPE.set_b(id, data.b)
	BUILDING_TYPE.set_construction_cost(id, data.construction_cost)
	if data.movable ~= nil then
		BUILDING_TYPE.set_movable(id, data.movable)
	end
	if RAWS_MANAGER.building_types_by_name[data.name] ~= nil then
		local msg = "Failed to load a building types (" .. tostring(data.name) .. ")"
		print(msg)
		error(msg)
	end
	RAWS_MANAGER.building_types_by_name[data.name] = id
	return id
end

---@param building_type building_type_id
---@return string
function BuildingType.get_tooltip(building_type)
	local fat = DATA.fatten_building_type(building_type)

	local s = fat.description

	s = s .. "\n\nBase cost: " .. tostring(fat.construction_cost) .. MONEY_SYMBOL
	s = s .. "\n\nUpkeep: " .. tostring(fat.upkeep) .. MONEY_SYMBOL

	if fat.production_method ~= INVALID_ID then
		local prod_method = fat.production_method
		do
			local new_string = ""
			local job_found = false
			new_string = new_string .. "\n\nJobs: "

			local job = DATA.production_method_get_job(prod_method)
			local amount = 1
			---@type string
			new_string = new_string .. DATA.job_get_description(job) .. " (" .. tostring(amount) .. "), "
			job_found = true

			if job_found then
				s = s .. new_string
			end
		end

		do
			local new_string = ""
			local found = false
			new_string = new_string .. "\n\nInputs: "
			for i = 1, MAX_SIZE_ARRAYS_PRODUCTION_METHOD do
				local use = DATA.production_method_get_inputs_use(prod_method, i)
				if use == INVALID_ID then
					break
				end
				local amount = DATA.production_method_get_inputs_amount(prod_method, i)
				---@type string
				new_string = new_string .. DATA.use_case_get_description(use) .. " (" .. tostring(amount) .. "), "
				found = true
			end

			if found then
				s = s .. new_string
			end
		end

		do
			local new_string = ""
			local found = false
			new_string = new_string .. "\n\nOutputs: "
			for i = 1, MAX_SIZE_ARRAYS_PRODUCTION_METHOD do
				local good = DATA.production_method_get_outputs_good(prod_method, i)
				if good == INVALID_ID then
					break
				end
				local amount = DATA.production_method_get_outputs_amount(prod_method, i)
				---@type string
				new_string = new_string .. DATA.trade_good_get_description(good) .. " (" .. tostring(amount) .. "), "
				found = true
			end

			if found then
				s = s .. new_string
			end
		end
	end

	if fat.needed_infrastructure > 0 then
		s = s .. "\n\nNeeded infrastructure: " .. tostring(fat.needed_infrastructure)
	end

	if fat.spotting > 0 then
		s = s .. "\n\nSpotting: " .. tostring(fat.spotting)
	end

	return s
end

return BuildingType
