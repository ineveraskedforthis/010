require "codegen-lua.use_case"

---@class UseCase
local UseCase = {}

---@class (exact) use_case_id_data_blob_definition
---@field name string
---@field icon string
---@field description string
---@field good_consumption number
---@field r number
---@field g number
---@field b number

---Creates a new trade good
---@param o use_case_id_data_blob_definition
---@return use_case_id
function UseCase:new(data)
	if RAWS_MANAGER.do_logging then
		print("Trade Good Use Case: " .. tostring(data.name))
	end

	local id = USE_CASE.create()
	USE_CASE.set_good_consumption(id, data.good_consumption)
	USE_CASE.set_r(id, data.r)
	USE_CASE.set_g(id, data.g)
	USE_CASE.set_b(id, data.b)

	if RAWS_MANAGER.use_cases_by_name[data.name] ~= nil then
		local msg = "Failed to load a trade good use case (" .. tostring(data.name) .. ")"
		print(msg)
		error(msg)
	end

	RAWS_MANAGER.use_cases_by_name[data.name] = id
	return id
end

return UseCase
