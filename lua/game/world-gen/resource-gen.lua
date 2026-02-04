local ffi = require "ffi"

local ge = {}

function ge.run()
	for _, res_id in pairs(RAWS_MANAGER.resources_by_name) do
		ffi.C.apply_resource(res_id)
	end
end

return ge
