require "codegen-lua.trade_good"

---@class (exact) trade_good_id_data_blob_definition
---@field name string
---@field icon string
---@field description string
---@field r number
---@field g number
---@field b number
---@field is_service boolean?
---@field base_price number
---@field decay number

local TradeGood = {}
---Creates a new trade good
---@param data trade_good_id_data_blob_definition
---@return trade_good_id
function TradeGood:new(data)
	if RAWS_MANAGER.do_logging then
		print("Trade Good: " .. tostring(data.name))
	end

	local id = TRADE_GOOD.create()

	TRADE_GOOD.set_r(id, data.r)
	TRADE_GOOD.set_g(id, data.g)
	TRADE_GOOD.set_b(id, data.b)
	if (data.is_service) then
		TRADE_GOOD.set_is_service(id, data.is_service)
	end
	TRADE_GOOD.set_base_price(id, data.base_price)
	TRADE_GOOD.set_decay(id, data.decay)

	if RAWS_MANAGER.trade_goods_by_name[data.name] ~= nil then
		local msg = "Failed to load a trade good (" .. tostring(data.name) .. ")"
		print(msg)
		error(msg)
	end

	RAWS_MANAGER.trade_goods_by_name[data.name] = id

	return id
end

return TradeGood
