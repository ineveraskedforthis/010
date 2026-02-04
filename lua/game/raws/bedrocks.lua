local col = require "game.color"
local ffi = require "ffi"

require "codegen-lua.bedrock"

---@class (exact) bedrock_id_o_blob_definition
---@field name string
---@field r number
---@field g number
---@field b number
---@field sand number
---@field silt number
---@field clay number
---@field organics number
---@field minerals number
---@field weathering number
---@field grain_size number?
---@field acidity number?
---@field igneous_extrusive boolean?
---@field igneous_intrusive boolean?
---@field sedimentary boolean?
---@field clastic boolean?
---@field evaporative boolean?
---@field metamorphic_marble boolean?
---@field metamorphic_slate boolean?
---@field oceanic boolean?
---@field sedimentary_ocean_deep boolean?
---@field sedimentary_ocean_shallow boolean?

local Bedrock = {}
Bedrock.__index = Bedrock
---@param o bedrock_id_o_blob_definition
---@return bedrock_id
function Bedrock:new(o)
	local id = BEDROCK.create()
	BEDROCK.set_grain_size(id, 0.0)
	BEDROCK.set_acidity(id, 0.0)
	BEDROCK.set_igneous_extrusive(id, false)
	BEDROCK.set_igneous_intrusive(id, false)
	BEDROCK.set_sedimentary(id, false)
	BEDROCK.set_clastic(id, false)
	BEDROCK.set_evaporative(id, false)
	BEDROCK.set_metamorphic_marble(id, false)
	BEDROCK.set_metamorphic_slate(id, false)
	BEDROCK.set_oceanic(id, false)
	BEDROCK.set_sedimentary_ocean_deep(id, false)
	BEDROCK.set_sedimentary_ocean_shallow(id, false)
	BEDROCK.set_r(id, o.r)
	BEDROCK.set_g(id, o.g)
	BEDROCK.set_b(id, o.b)
	BEDROCK.set_sand(id, o.sand)
	BEDROCK.set_silt(id, o.silt)
	BEDROCK.set_clay(id, o.clay)
	BEDROCK.set_organics(id, o.organics)
	BEDROCK.set_minerals(id, o.minerals)
	BEDROCK.set_weathering(id, o.weathering)
	if o.grain_size ~= nil then
		BEDROCK.set_grain_size(id, o.grain_size)
	end
	if o.acidity ~= nil then
		BEDROCK.set_acidity(id, o.acidity)
	end
	if o.igneous_extrusive ~= nil then
		BEDROCK.set_igneous_extrusive(id, o.igneous_extrusive)
	end
	if o.igneous_intrusive ~= nil then
		BEDROCK.set_igneous_intrusive(id, o.igneous_intrusive)
	end
	if o.sedimentary ~= nil then
		BEDROCK.set_sedimentary(id, o.sedimentary)
	end
	if o.clastic ~= nil then
		BEDROCK.set_clastic(id, o.clastic)
	end
	if o.evaporative ~= nil then
		BEDROCK.set_evaporative(id, o.evaporative)
	end
	if o.metamorphic_marble ~= nil then
		BEDROCK.set_metamorphic_marble(id, o.metamorphic_marble)
	end
	if o.metamorphic_slate ~= nil then
		BEDROCK.set_metamorphic_slate(id, o.metamorphic_slate)
	end
	if o.oceanic ~= nil then
		BEDROCK.set_oceanic(id, o.oceanic)
	end
	if o.sedimentary_ocean_deep ~= nil then
		BEDROCK.set_sedimentary_ocean_deep(id, o.sedimentary_ocean_deep)
	end
	if o.sedimentary_ocean_shallow ~= nil then
		BEDROCK.set_sedimentary_ocean_shallow(id, o.sedimentary_ocean_shallow)
	end

	print(o.name)
	if RAWS_MANAGER.bedrocks_by_name[o.name] ~= nil then
		local msg = "Failed to load bedrock type (" .. tostring(o.name) .. ")"
		print(msg)
		error(msg)
	end
	RAWS_MANAGER.bedrocks_by_name[o.name] = id
	local cid = col.rgb_to_id(o.r, o.g, o.b)
	RAWS_MANAGER.bedrocks_by_color_id[cid] = id

	return id
end

return Bedrock
