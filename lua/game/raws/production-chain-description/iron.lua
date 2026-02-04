local TradeGood = require "game.raws.trade-goods"
local BuildingType = require "game.raws.building-types"
local ProductionMethod = require "game.raws.production-methods"

local prod = require "game.raws.raws-utils".production_method
local tec = require "game.raws.raws-utils".technology
local good = require "game.raws.raws-utils".trade_good
local res = require "game.raws.raws-utils".resource
local job = require "game.raws.raws-utils".job
local use = require "game.raws.raws-utils".trade_good_use_case
local add_use_case = require "game.raws.raws-utils".add_use_case

return function ()
	--- BASIC IRON PRODUCTION
	local r = 0.2
	local g = 0.2
	local b = 0.4

	-- for now there is no real iron mining, so we could just require iron meteorits to produce early advanced tools
	TradeGood:new {
		name = "tools-meteoric-iron",
		description = "meteoric iron tool",
		icon = "stone-axe.png",
		r = r,
		g = g,
		b = b,
		base_price = 10,

		decay = 1
	}
	add_use_case("tools-meteoric-iron", "tools-like", 3)
	add_use_case("tools-meteoric-iron", "tools", 2)
	add_use_case("tools-meteoric-iron", "tools-advanced", 1)
	ProductionMethod:new {
		name = "smith-tools-meteoric-iron",
		description = "cold smithing meteoric iron into tools",
		icon = "gold-nuggets.png",
		r = r,
		g = g,
		b = b,
		inputs = { [use("tools-like")] = 0.25 },
		outputs = { [good("tools-meteoric-iron")] = 0.1 },
		job = job("miners"),
		job_type = JOBTYPE.ARTISAN,
	}
	BuildingType:new {
		name = 'smith-tools-meteoric-iron',
		description = 'meteoric iron smiths',
		icon = 'anvil.png',
		r = r,
		g = g,
		b = b,
		unlocked_by = tec('early-metal-working'),
		production_method = prod('smith-tools-meteoric-iron'),
		needed_infrastructure = 2,
		construction_cost = 80,
		ai_weight = 10,
		required_biome = {},
		required_resource = {res("meteoric-iron")},
	}
end