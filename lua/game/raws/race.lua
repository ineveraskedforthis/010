local ffi = require("ffi")
require "codegen-lua.race"

---@class struct_need_definition
---@field need NEED
---@field use_case use_case_id
---@field required number
ffi.cdef[[
	typedef struct {
		uint8_t need;
		int32_t use_case;
		float required;
	} need_definition;
]]


---@class (exact) PortraitSet
---@field fallback PortraitDescription
---@field child PortraitDescription?
---@field teen PortraitDescription?
---@field adult PortraitDescription?
---@field middle PortraitDescription?
---@field elder PortraitDescription?

---@class (exact) PortraitDescription
---@field folder string
---@field layers string[]
---@field layers_groups string[][]

require "codegen-lua.portrait_set"
require "codegen-lua.portrait_layer"
require "codegen-lua.portrait_layer_group"

---@param desc PortraitDescription
---@return portrait_set_id
local function portrait_setup(desc)
	local id = PORTRAIT_SET.create()
	local path = "lua/portraits/"
	local folder = desc.folder .. "/"

	---@type table<string, portrait_layer_id>
	local name_to_layer = {}

	for index, value in ipairs(desc.layers) do
		local layer = PORTRAIT_LAYER.create()
		local true_path = path .. folder .. value
		PORTRAIT_LAYER.set_path_text_index(layer, ffi.C.register_texture(true_path:len(), true_path))
		name_to_layer[value] = layer
		PORTRAIT_SET.set_layers(id, index - 1, layer)
	end

	local group_index = 0
	for index, value in pairs(desc.layers_groups) do
		local group = PORTRAIT_LAYER_GROUP.create()
		local index_in_group = 0
		for _, layer_string in pairs(value) do
			PORTRAIT_LAYER_GROUP.set_group(group, index_in_group, name_to_layer[layer_string])
			index_in_group = index_in_group + 1
		end
		PORTRAIT_SET.set_groups(id, group_index, group)
		group_index = group_index + 1
	end

	return id
end

local Race = {}
Race.__index = Race

---@class (exact) race_id_data_blob_definition
---@field name string
---@field icon string
---@field female_portrait nil|PortraitSet
---@field male_portrait nil|PortraitSet
---@field description string
---@field r number
---@field g number
---@field b number
---@field carrying_capacity_weight number
---@field fecundity number
---@field spotting number How good is this unit at scouting
---@field visibility number How visible is this unit in battles
---@field males_per_hundred_females number
---@field child_age number
---@field teen_age number
---@field adult_age number
---@field middle_age number
---@field elder_age number
---@field max_age number
---@field minimum_comfortable_temperature number
---@field minimum_absolute_temperature number
---@field minimum_comfortable_elevation number?
---@field female_body_size number
---@field male_body_size number
---@field female_efficiency number[]
---@field male_efficiency number[]
---@field female_infrastructure_needs number
---@field male_infrastructure_needs number
---@field requires_large_river boolean?
---@field requires_large_forest boolean?

---@class race_id_data_blob_definition_extended : race_id_data_blob_definition
---@field female_needs table<NEED, table<use_case_id, number>>
---@field male_needs table<NEED, table<use_case_id, number>>

---@param o race_id_data_blob_definition_extended
function Race:new(o)
	local r = RACE.create()

	-- assert that needs are valid
	---@type struct_need_definition[]
	local male_needs = {}
	---@type struct_need_definition[]
	local female_needs = {}

	for need, uses_table in pairs(o.male_needs) do
		for use_case, value in pairs(uses_table) do
			table.insert(male_needs, {
				need = need,
				use_case = use_case,
				required = value
			})
		end
	end

	for need, uses_table in pairs(o.female_needs) do
		for use_case, value in pairs(uses_table) do
			table.insert(female_needs, {
				need = need,
				use_case = use_case,
				required = value
			})
		end
	end

	--- check that they are consistent:

	print("needs")
	print(o.name)

	for i = 1, math.max(#male_needs, #female_needs) do
		assert(male_needs[i].need == female_needs[i].need)
		assert(male_needs[i].use_case == female_needs[i].use_case)
		assert(male_needs[i].required > 0)
		assert(female_needs[i].required > 0)

		local need = male_needs[i].need
		local use_case = male_needs[i].use_case

		print(i, need, use_case)

		RACE.set_needs_category(r, i, need)
		RACE.set_needs_use_case(r, i, use_case)
		RACE.set_male_needs_amount(r, i, male_needs[i].required)
		RACE.set_female_needs_amount(r, i, female_needs[i].required)
	end

	local text_key = ffi.C.register_text(string.len(o.name), o.name)
	RACE.set_name_text_index(r, text_key)

	local icon_path = PATH_TO_ASSETS_FOLDER ..  o.icon
	local icon_key = ffi.C.register_texture(string.len(icon_path), icon_path)
	RACE.set_icon_path_text_index(r, icon_key)

	RACE.set_minimum_comfortable_elevation(r, 0.0)
	RACE.set_requires_large_river(r, false)
	RACE.set_requires_large_forest(r, false)
	RACE.set_r(r, o.r)
	RACE.set_g(r, o.g)
	RACE.set_b(r, o.b)
	RACE.set_carrying_capacity_weight(r, o.carrying_capacity_weight)
	RACE.set_fecundity(r, o.fecundity)
	RACE.set_spotting(r, o.spotting)
	RACE.set_visibility(r, o.visibility)
	RACE.set_males_per_hundred_females(r, o.males_per_hundred_females)
	RACE.set_child_age(r, o.child_age)
	RACE.set_teen_age(r, o.teen_age)
	RACE.set_adult_age(r, o.adult_age)
	RACE.set_middle_age(r, o.middle_age)
	RACE.set_elder_age(r, o.elder_age)
	RACE.set_max_age(r, o.max_age)
	RACE.set_minimum_comfortable_temperature(r, o.minimum_comfortable_temperature)
	RACE.set_minimum_absolute_temperature(r, o.minimum_absolute_temperature)
	if o.minimum_comfortable_elevation ~= nil then
		RACE.set_minimum_comfortable_elevation(r, o.minimum_comfortable_elevation)
	end
	RACE.set_female_body_size(r, o.female_body_size)
	RACE.set_male_body_size(r, o.male_body_size)
	for i, value in pairs(o.female_efficiency) do
		RACE.set_female_efficiency(r, i, value)
	end
	for i, value in pairs(o.male_efficiency) do
		RACE.set_male_efficiency(r, i, value)
	end
	RACE.set_female_infrastructure_needs(r, o.female_infrastructure_needs)
	RACE.set_male_infrastructure_needs(r, o.male_infrastructure_needs)
	if o.requires_large_river ~= nil then
		RACE.set_requires_large_river(r, o.requires_large_river)
	end
	if o.requires_large_forest ~= nil then
		RACE.set_requires_large_forest(r, o.requires_large_forest)
	end

	-- handle portraits
	if (o.male_portrait.fallback) then
		RACE.set_portrait_fallback_male(r, portrait_setup(o.male_portrait.fallback))
		print("male portrait " .. o.name)
	end
	if (o.female_portrait.fallback) then
		RACE.set_portrait_fallback_female(r, portrait_setup(o.female_portrait.fallback))
		print("female portrait " .. o.name)
	end
	-- todo: the rest of the portraits

	if RAWS_MANAGER.races_by_name[o.name] ~= nil then
		local msg = "Failed to load a race (" .. tostring(o.name) .. ")"
		print(msg)
		error(msg)
	end

	RAWS_MANAGER.races_by_name[o.name] = r
	return r
end

return Race
