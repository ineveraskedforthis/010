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
	local r = 0.6
	local g = 0.84
	local b = 0.9

	local material_icon = "cubes.png"


	UseCase:new {
		name = "arsenic-bronze-source",	description = "source of arsenic bronze",
		icon = material_icon, r = r, g = g, b = b,
		good_consumption = 1
	}

	TradeGood:new {
		name = "arsenic-bronze",
		description = "arsenic bronze",
		icon = material_icon,
		r = r,
		g = g,
		b = b,
		base_price = 10,

		decay = 1
	}
	add_use_case("arsenic-bronze", "arsenic-bronze-source", 1)

	TradeGood:new {
		name = "tools-arsenic-bronze",
		description = "arsenic bronze tools",
		icon = "stone-axe.png",
		r = r,
		g = g,
		b = b,
		base_price = 10,

		decay = 1
	}
	add_use_case("tools-arsenic-bronze", "tools-like", 3)
	add_use_case("tools-arsenic-bronze", "tools", 2)
	add_use_case("tools-arsenic-bronze", "tools-advanced", 1)

	---producing alloy

	ProductionMethod:new {
		name = "smelting-arsenic-bronze", description = "smelting copper and arsenic into alloy",
		icon = material_icon, r = r, g = g,	b = b,
		inputs = { [use("tools")] = 0.25, [use("copper-source")] = 1, [use("structural-material")] = 0.1, [use("fuel")] = 5 },
		outputs = { [good("arsenic-bronze")] = 1 },
		job = job("smelters"), job_type = JOBTYPE.ARTISAN,
	}
	BuildingType:new {
		name = 'smelter-arsenic-bronze', description = 'smelter dedicated for arsenic bronze',
		icon = material_icon, r = r, g = g, b = b,
		unlocked_by = tec("arsenical-bronze"),
		production_method = prod('smelting-arsenic-bronze'),
		needed_infrastructure = 2, construction_cost = 80,
		required_biome = {}, required_resource = {res("arsenic")},
	}

	---producing tools out of alloy

	ProductionMethod:new {
		name = "smith-tools-arsenic-bronze", description = "making arsenic bronze tools",
		icon = material_icon, r = r, g = g,	b = b,
		inputs = { [use("tools")] = 0.25, [use("arsenic-bronze-source")] = 1 },
		outputs = { [good("tools-arsenic-bronze")] = 1 },
		job = job("miners"), job_type = JOBTYPE.ARTISAN,
	}
	BuildingType:new {
		name = 'smith-tools-arsenic-bronze', description = 'arsenic bronze tools smith',
		icon = material_icon, r = r, g = g, b = b,
		unlocked_by = tec("arsenical-bronze"),
		production_method = prod('smith-tools-arsenic-bronze'),
		needed_infrastructure = 2, construction_cost = 80,
		required_biome = {}, required_resource = {},
	}
end