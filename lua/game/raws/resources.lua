require "codegen-lua.resource"

local Resource = {}

---@class (exact) resource_id_data_blob_definition
---@field name string
---@field icon string
---@field description string
---@field r number
---@field g number
---@field b number
---@field required_biome biome_id[]
---@field required_bedrock bedrock_id[]
---@field base_frequency number? number of tiles per which this resource is spawned
---@field coastal boolean?
---@field land boolean?
---@field water boolean?
---@field ice_age boolean? requires presence of ice age ice
---@field minimum_trees number?
---@field maximum_trees number?
---@field minimum_elevation number?
---@field maximum_elevation number?

---Creates a new resource
---@param data resource_id_data_blob_definition
---@return resource_id
function Resource:new(data)
	local id = RESOURCE.create()

	if RAWS_MANAGER.do_logging then
		print("Resource: " .. tostring(id) .. " " .. tostring(data.name))
	end

	RESOURCE.set_base_frequency(id, 1000)
	RESOURCE.set_coastal(id, false)
	RESOURCE.set_land(id, true)
	RESOURCE.set_water(id, false)
	RESOURCE.set_ice_age(id, false)
	RESOURCE.set_minimum_trees(id, 0)
	RESOURCE.set_maximum_trees(id, 1)
	RESOURCE.set_minimum_elevation(id, -math.huge)
	RESOURCE.set_maximum_elevation(id, math.huge)
	RESOURCE.set_r(id, data.r)
	RESOURCE.set_g(id, data.g)
	RESOURCE.set_b(id, data.b)
	for i, value in pairs(data.required_biome) do
		RESOURCE.set_required_biome(id, i, value)
	end
	for i, value in pairs(data.required_bedrock) do
		RESOURCE.set_required_bedrock(id, i, value)
	end
	if data.base_frequency ~= nil then
		RESOURCE.set_base_frequency(id, data.base_frequency)
	end
	if data.coastal ~= nil then
		RESOURCE.set_coastal(id, data.coastal)
	end
	if data.land ~= nil then
		RESOURCE.set_land(id, data.land)
	end
	if data.water ~= nil then
		RESOURCE.set_water(id, data.water)
	end
	if data.ice_age ~= nil then
		RESOURCE.set_ice_age(id, data.ice_age)
	end
	if data.minimum_trees ~= nil then
		RESOURCE.set_minimum_trees(id, data.minimum_trees)
	end
	if data.maximum_trees ~= nil then
		RESOURCE.set_maximum_trees(id, data.maximum_trees)
	end
	if data.minimum_elevation ~= nil then
		RESOURCE.set_minimum_elevation(id, data.minimum_elevation)
	end
	if data.maximum_elevation ~= nil then
		RESOURCE.set_maximum_elevation(id, data.maximum_elevation)
	end

	if RAWS_MANAGER.resources_by_name[data.name] ~= nil then
		local msg = "Failed to load a resource (" .. tostring(data.name) .. ")"
		print(msg)
		error(msg)
	end
	RAWS_MANAGER.resources_by_name[data.name] = id

	return id
end

return Resource
