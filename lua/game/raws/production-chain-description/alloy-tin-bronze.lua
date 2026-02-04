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

--- for now production of alloys would be local to sources of added materials
--- TODO: add mining of ores to allow trade

return function ()
	local r = 0.31
	local g = 1.0
	local b = 0.05

	local material_icon = "cubes.png"

	UseCase:new {
		name = "tin-source",	description = "source of tin",
		icon = material_icon, r = r, g = g, b = b,
		good_consumption = 1
	}
	TradeGood:new {
		name = "ingot-tin",
		description = "tin",
		icon = material_icon,
		r = r,
		g = g,
		b = b,
		base_price = 10,

		decay = 1
	}
	add_use_case("ingot-tin", "tin-source", 1)

	UseCase:new {
		name = "tin-bronze-source",	description = "source of tin bronze",
		icon = material_icon, r = r, g = g, b = b,
		good_consumption = 1
	}
	TradeGood:new {
		name = "ingot-tin-bronze",
		description = "tin bronze",
		icon = material_icon,
		r = r,
		g = g,
		b = b,
		base_price = 10,

		decay = 1
	}
	add_use_case("ingot-tin-bronze", "tin-bronze-source", 1)

	TradeGood:new {
		name = "tools-tin-bronze",
		description = "tin bronze tools",
		icon = "stone-axe.png",
		r = r,
		g = g,
		b = b,
		base_price = 10,

		decay = 1
	}
	add_use_case("tools-tin-bronze", "tools-like", 3)
	add_use_case("tools-tin-bronze", "tools", 2)
	add_use_case("tools-tin-bronze", "tools-advanced", 1)

	---producing alloy

	ProductionMethod:new {
		name = "smelting-tin-bronze", description = "smelting copper and tin into alloy",
		icon = material_icon, r = r, g = g,	b = b,
		inputs = { [use("tools")] = 0.25, [use("copper-source")] = 0.875, [use("tin-source")] = 0.125, [use("structural-material")] = 0.1, [use("fuel")] = 5},
		outputs = { [good("ingot-tin-bronze")] = 1 },
		job = job("smelters"), job_type = JOBTYPE.ARTISAN,
	}
	BuildingType:new {
		name = 'smelter-tin-bronze', description = 'smelter dedicated for tin bronze',
		icon = material_icon, r = r, g = g, b = b,
		unlocked_by = tec("tin-bronze"),
		production_method = prod('smelting-tin-bronze'),
		needed_infrastructure = 2, construction_cost = 80,
		required_biome = {}, required_resource = {res("tin")},
	}

	---producing tools out of alloy

	ProductionMethod:new {
		name = "smith-tools-tin-bronze", description = "making tin bronze tools",
		icon = material_icon, r = r, g = g,	b = b,
		inputs = { [use("tools")] = 0.25, [use("tin-bronze-source")] = 1 },
		outputs = { [good("tools-tin-bronze")] = 1 },
		job = job("miners"), job_type = JOBTYPE.ARTISAN,
	}
	BuildingType:new {
		name = 'smith-tools-tin-bronze', description = 'tin bronze tools smith',
		icon = material_icon, r = r, g = g, b = b,
		unlocked_by = tec("tin-bronze"),
		production_method = prod('smith-tools-tin-bronze'),
		needed_infrastructure = 2, construction_cost = 80,
		required_biome = {}, required_resource = {},
	}
end