require "codegen-lua.use_weight"

local ut = {}


---@param id string
---@return job_id
function ut.job(id)
	local r = RAWS_MANAGER.jobs_by_name[id]
	if r == nil then
		print("Job " .. id .. " doesn't exist!")
		error("Job " .. id .. " doesn't exist!")
		love.event.quit()
	end

	return r
end

---@param id string
---@return trade_good_id
function ut.trade_good(id)
	local r = RAWS_MANAGER.trade_goods_by_name[id]
	if r == nil then
		print("Trade good " .. id .. " doesn't exist!")
		error("Trade good " .. id .. " doesn't exist!")
		love.event.quit()
	end

	return r
end

---@param id string
---@return use_case_id
function ut.trade_good_use_case(id)
	local r = RAWS_MANAGER.use_cases_by_name[id]
	if r == nil then
		print("Use case " .. id .. " doesn't exist!")
		error("Use case " .. id .. " doesn't exist!")
		love.event.quit()
	end

	return r
end

---@param id string
---@return technology_id
function ut.technology(id)
	local r = RAWS_MANAGER.technologies_by_name[id]
	if r == nil then
		print("Technology " .. id .. " doesn't exist!")
		error("Technology " .. id .. " doesn't exist!")
		love.event.quit()
	end

	return r
end

---@param id string
---@return race_id
function ut.race(id)
	local r = RAWS_MANAGER.races_by_name[id]
	if r == nil then
		print("Race " .. id .. " doesn't exist!")
		error("Race " .. id .. " doesn't exist!")
		love.event.quit()
	end

	return r
end

---@param id string
---@return production_method_id
function ut.production_method(id)
	local r = RAWS_MANAGER.production_methods_by_name[id]
	if r == nil then
		print("Production method " .. id .. " doesn't exist!")
		error("Production method " .. id .. " doesn't exist!")
		love.event.quit()
	end

	return r
end

---@param id string
---@return biome_id
function ut.biome(id)
	local r = RAWS_MANAGER.biomes_by_name[id]
	if r == nil then
		print("Biome " .. id .. " doesn't exist!")
		error("Biome " .. id .. " doesn't exist!")
		love.event.quit()
	end
	return r
end

---@param id string
---@return bedrock_id
function ut.bedrock(id)
	local r = RAWS_MANAGER.bedrocks_by_name[id]
	if r == nil then
		print("Bedrock " .. id .. " doesn't exist!")
		error("Bedrock " .. id .. " doesn't exist!")
		love.event.quit()
	end
	return r
end

---@param id string
---@return BiogeographicRealm
function ut.biogeographic_realm(id)
	local r = RAWS_MANAGER.biogeographic_realms_by_name[id]
	if r == nil then
		print("Biogeographic realm " .. id .. " doesn't exist!")
		error("Biogeographic realm " .. id .. " doesn't exist!")
		love.event.quit()
	end

	return r
end

---@param id string
---@return resource_id
function ut.resource(id)
	local r = RAWS_MANAGER.resources_by_name[id]
	if r == nil then
		print("Resource " .. id .. " doesn't exist!")
		error("Resource " .. id .. " doesn't exist!")
		love.event.quit()
	end

	return r
end

---@param id string
---@return unit_type_id
function ut.unit_type(id)
	local r = RAWS_MANAGER.unit_types_by_name[id]
	if r == nil then
		print("Unit Type " .. id .. " doesn't exist!")
		error("Unit Type " .. id .. " doesn't exist!")
		love.event.quit()
	end

	return r
end

---@param id string
---@return Event
function ut.event(id)
	local r = RAWS_MANAGER.events_by_name[id]
	if r == nil then
		print("Event " .. id .. " doesn't exist!")
		error("Event " .. id .. " doesn't exist!")
		love.event.quit()
	end

	return r
end

---@param x string
---@return fun(): string
function ut.constant_string(x)
	return function() return x end
end

---Adds a trade good to a use case
---@param trade_good string
---@param use_case string
---@param weight number
function ut.add_use_case(trade_good, use_case, weight)
	print("use case:", trade_good, use_case, weight)
	local retrieved_use_case = ut.trade_good_use_case(use_case)
	local retrieved_trade_good = ut.trade_good(trade_good)
	local use_weight = USE_WEIGHT. force_create(retrieved_trade_good, retrieved_use_case)
	print(retrieved_use_case, retrieved_trade_good, use_weight)
	USE_WEIGHT.set_weight(use_weight, weight)
end

return ut
