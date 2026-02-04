require "codegen-lua.technology_unlock"
require "codegen-lua.technology"

local tabb = require "engine.table"

---@class (exact) technology_id_data_blob_definition
---@field name string
---@field icon string
---@field description string
---@field r number
---@field g number
---@field b number
---@field research_cost number Amount of research points (education_endowment) per pop needed for the technology
---@field required_biome biome_id[]
---@field required_race race_id[]
---@field required_resource resource_id[]
---@field associated_job job_id The job that is needed to perform this research. Without it, the research odds will be significantly lower. We'll be using this to make technology implicitly tied to player decisions
---@field throughput_boosts table<production_method_id, number>
---@field input_efficiency_boosts table<production_method_id, number>
---@field output_efficiency_boosts table<production_method_id, number>

---@class technology_blob_extended : technology_id_data_blob_definition
---@field unlocked_by technology_id[]


local Technology = {}

---Creates a new technology
---commenting
---@param data technology_blob_extended
---@return technology_id
function Technology:new(data)
	if RAWS_MANAGER.do_logging then
		print("Technology: " .. tostring(data.name))
	end

	local id = TECHNOLOGY.create()
	-- TECHNOLOGY.set_name(id, data.name)
	-- TECHNOLOGY.set_icon(id, data.icon)
	-- TECHNOLOGY.set_description(id, data.description)
	TECHNOLOGY.set_r(id, data.r)
	TECHNOLOGY.set_g(id, data.g)
	TECHNOLOGY.set_b(id, data.b)
	TECHNOLOGY.set_research_cost(id, data.research_cost)
	for i, value in pairs(data.required_biome) do
		TECHNOLOGY.set_required_biome(id, i, value)
	end
	for i, value in pairs(data.required_race) do
		TECHNOLOGY.set_required_race(id, i, value)
	end
	for i, value in pairs(data.required_resource) do
		TECHNOLOGY.set_required_resource(id, i, value)
	end
	TECHNOLOGY.set_associated_job(id, data.associated_job)
	for i, value in pairs(data.throughput_boosts) do
		TECHNOLOGY.set_throughput_boosts(id, i, value)
	end
	for i, value in pairs(data.input_efficiency_boosts) do
		TECHNOLOGY.set_input_efficiency_boosts(id, i, value)
	end
	for i, value in pairs(data.output_efficiency_boosts) do
		TECHNOLOGY.set_output_efficiency_boosts(id, i, value)
	end

	for _, item in pairs(data.unlocked_by) do
		TECHNOLOGY_UNLOCK.force_create(item, id)
	end

	-- if ASSETS.icons[data.icon] == nil then
	-- 	print("Missing icon: " .. data.icon)
	-- 	error("Technology " .. data.name .. " has no icon. " .. "Missing icon: " .. data.icon)
	-- end

	if RAWS_MANAGER.technologies_by_name[data.name] ~= nil then
		local msg = "Failed to load a technology (" .. tostring(data.name) .. ")"
		print(msg)
		error(msg)
	end
	RAWS_MANAGER.technologies_by_name[data.name] = id

	return id
end

---Generates tooltip for a given technology
---@param technology technology_id
---@return string
function Technology.get_tooltip(technology)
	local technology_data = DATA.fatten_technology(technology)

	local s = technology_data.description .. "\n\n"

	s = s .. "Difficulty: " .. tostring(technology_data.research_cost) .. "\n"
	if technology_data.associated_job then
		s = s .. "\nAssociated job: " .. DATA.job_get_name(technology_data.associated_job)
	end

	do
		local requires = false
		local string = ""
		string = string .. "\n Required biome: "
		for i = 1, MAX_REQUIREMENTS_TECHNOLOGY - 1 do
			local thing = TECHNOLOGY.get_required_biome(technology, i)
			if thing == INVALID_ID then
				break
			end
			print(technology_data.description)
			print(thing)
			local name = DATA.biome_get_name(thing)
			string = string .. name .. ", "
			requires = true
		end
		string = string .. "\n"
		if requires then
			s = s .. string
		end
	end

	do
		local requires = false
		local string = ""
		string = string .. "\n Required race: "
		for i = 1, MAX_REQUIREMENTS_TECHNOLOGY - 1 do
			local thing = TECHNOLOGY.get_required_race(technology, i)
			if thing == INVALID_ID then
				break
			end
			local name = DATA.race_get_name(thing)
			string = string .. name .. ", "
			requires = true
		end
		string = string .. "\n"
		if requires then
			s = s .. string
		end
	end

	do
		local requires = false
		local string = ""
		string = string .. "\n Required resource: "
		for i = 1, MAX_REQUIREMENTS_TECHNOLOGY - 1 do
			local thing = TECHNOLOGY.get_required_resource(technology, i)
			if thing == INVALID_ID then
				break
			end
			local name = DATA.resource_get_name(thing)
			string = string .. name .. ", "
			requires = true
		end
		string = string .. "\n"
		if requires then
			s = s .. string
		end
	end

	do
		local requires = false
		local string = ""
		string = string .. "\n Unlocked buildings: "
		for _, item in ipairs(DATA.get_technology_building_from_technology(technology)) do
			local name = DATA.building_type_get_description(TECHNOLOGY.building_get_unlocked(item))
			string = string .. name .. ", "
			requires = true
		end
		string = string .. "\n"
		if requires then
			s = s .. string
		end
	end

	do
		local requires = false
		local string = ""
		string = string .. "\n Unlocked units: "
		for _, item in ipairs(DATA.get_technology_unit_from_technology(technology)) do
			local name = DATA.unit_type_get_name(TECHNOLOGY.unit_get_unlocked(item))
			string = string .. name .. ", "
			requires = true
		end
		string = string .. "\n"
		if requires then
			s = s .. string
		end
	end

	do
		local requires = false
		local string = ""
		string = string .. "\n Unlocked technology paths: "
		local thing = DATA.get_technology_unlock_from_origin(technology)
		for _, i in ipairs(thing) do
			local name = TECHNOLOGY.get_name(TECHNOLOGY.unlock_get_unlocked(i))
			string = string .. name .. ", "
			requires = true
		end
		string = string .. "\n"
		if requires then
			s = s .. string
		end
	end

	do
		local requires = false
		local string = ""
		string = string .. "\n Unlocked by: "
		local thing = DATA.get_technology_unlock_from_unlocked(technology)
		for _, i in ipairs(thing) do
			local name = TECHNOLOGY.get_name(TECHNOLOGY.unlock_get_origin(i))
			string = string .. name .. ", "
			requires = true
		end
		string = string .. "\n"
		if requires then
			s = s .. string
		end
	end

	do
		local requires = false
		local string = ""
		string = string .. "\n Throughput: "

		local function build_string(production_method_id)
			local thing = TECHNOLOGY.get_throughput_boosts(technology, production_method_id)
			local name = DATA.production_method_get_name(production_method_id)
			if thing ~= 0 then
				string = string .. name .. " (+" .. tostring(math.floor(100 * thing)) .. "%), "
				requires = true
			end
		end

		DATA.for_each_production_method(build_string)

		string = string .. "\n"
		if requires then
			s = s .. string
		end
	end

	do
		local requires = false
		local string = ""
		string = string .. "\n Input: "

		local function build_string(production_method_id)
			local thing = TECHNOLOGY.get_input_efficiency_boosts(technology, production_method_id)
			local name = DATA.production_method_get_name(production_method_id)
			if thing ~= 0 then
				string = string .. name .. " (+" .. tostring(math.floor(100 * thing)) .. "%), "
				requires = true
			end
		end

		DATA.for_each_production_method(build_string)

		string = string .. "\n"
		if requires then
			s = s .. string
		end
	end

	do
		local requires = false
		local string = ""
		string = string .. "\n Output: "

		local function build_string(production_method_id)
			local thing = TECHNOLOGY.get_output_efficiency_boosts(technology, production_method_id)
			local name = DATA.production_method_get_name(production_method_id)
			if thing ~= 0 then
				string = string .. name .. " (+" .. tostring(math.floor(100 * thing)) .. "%), "
				requires = true
			end
		end

		DATA.for_each_production_method(build_string)

		string = string .. "\n"
		if requires then
			s = s .. string
		end
	end

	return s
end

return Technology
