require "codegen-lua.job"

local Job = {}


---@class (exact) job_id_data_blob_definition
---@field name string
---@field icon string
---@field description string
---@field r number
---@field g number
---@field b number

---Creates a new job
---@param o job_id_data_blob_definition
---@return job_id
function Job:new(data)
	local id = JOB.create()
	JOB.set_r(id, data.r)
	JOB.set_g(id, data.g)
	JOB.set_b(id, data.b)

	if RAWS_MANAGER.jobs_by_name[data.name] ~= nil then
		local msg = "Failed to load a job (" .. tostring(data.name) .. ")"
		print(msg)
		error(msg)
	end
	RAWS_MANAGER.jobs_by_name[data.name] = id
	return id
end

return Job
