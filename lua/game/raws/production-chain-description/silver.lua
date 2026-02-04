local TradeGood = require "game.raws.trade-goods"
local BuildingType = require "game.raws.building-types"
local ProductionMethod = require "game.raws.production-methods"
local UseCase = require "game.raws.use-case"

local prod = require "game.raws.raws-utils".production_method
local tec = require "game.raws.raws-utils".technology
local good = require "game.raws.raws-utils".trade_good
local res = require "game.raws.raws-utils".resource
local job = require "game.raws.raws-utils".job
local use = require "game.raws.raws-utils".trade_good_use_case
local add_use_case = require "game.raws.raws-utils".add_use_case

return function ()
	local r = 1.00
	local g = 0.84
	local b = 1.00

	local material_icon = "cubes.png"
	local jewelry_icon = "tribal-pendant.png"

	UseCase:new {
		name = "silver-source",
		description = "source of silver",
		icon = material_icon,
		good_consumption = 1,
		r = r,
		g = g,
		b = b,
	}

	TradeGood:new {
		name = "silver",
		description = "silver",
		icon = material_icon,
		r = r,
		g = g,
		b = b,
		base_price = 10,

		decay = 1
	}
	add_use_case("silver", "jewelry", 0.01)
	add_use_case("silver", "silver-source", 1)

	TradeGood:new {
		name = "silver-jewelry",
		description = "silver jewelry",
		icon = jewelry_icon,
		r = r,
		g = g,
		b = b,
		base_price = 10,

		decay = 1
	}
	add_use_case("silver-jewelry", "jewelry", 1)
	add_use_case("silver-jewelry", "silver-source", 1)

	ProductionMethod:new {
		name = "gathering-native-silver",
		description = "gathering native silver",
		icon = "gold-nuggets.png",
		r = r,
		g = g,
		b = b,
		inputs = { [use("tools-like")] = 0.25 },
		outputs = { [good("silver")] = 1 },
		job = job("miners"),
		job_type = JOBTYPE.ARTISAN,
	}
	ProductionMethod:new {
		name = "mining-silver",
		description = "mining silver",
		icon = "war-pick.png",
		r = r,
		g = g,
		b = b,
		inputs = { [use("tools-advanced")] = 0.25 },
		outputs = { [good("silver")] = 1 },
		job = job("miners"),
		job_type = JOBTYPE.ARTISAN,
	}
	ProductionMethod:new {
		name = "smith-jewelry-silver",
		description = "making jewelry out of silver",
		icon = "war-pick.png",
		r = r,
		g = g,
		b = b,
		inputs = { [use("tools")] = 0.25, [use("silver-source")] = 1 },
		outputs = { [good("silver-jewelry")] = 1 },
		job = job("miners"),
		job_type = JOBTYPE.ARTISAN,
	}

	BuildingType:new {
		name = 'gathering-native-silver', description = 'gathering silver',	icon = 'gold-nuggets.png',
		r = r, g = g, b = b,
		unlocked_by = tec('early-metal-working'),
		production_method = prod('gathering-native-silver'),
		needed_infrastructure = 2,
		construction_cost = 80,
		ai_weight = 10,
		required_biome = {},
		required_resource = {res("native-silver")},
	}
	BuildingType:new {
		name = 'mining-silver', description = 'mining silver', icon = 'gold-nuggets.png',
		r = r, g = g, b = b,
		unlocked_by = tec('surface-mining'), production_method = prod('mining-silver'),
		needed_infrastructure = 2,
		construction_cost = 80,
		ai_weight = 10,
		required_biome = {},
		required_resource = {res("silver")},
	}
	BuildingType:new {
		name = "smith-jewelry-silver", description = "silversmith", icon = "tribal-pendant.png",
		r = r, g = g, b = b,
		unlocked_by = tec("early-metal-working"), production_method = prod("smith-jewelry-silver"),
		needed_infrastructure = 2, construction_cost = 80,
		required_biome = {}, required_resource = {},
	}
end