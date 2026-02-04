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
	local r = 0.70
	local g = 0.60
	local b = 0.10

	local material_icon = "cubes.png"
	local jewelry_icon = "tribal-pendant.png"

	UseCase:new {
		name = "brass-source",	description = "source of brass",
		icon = material_icon, r = r, g = g, b = b,
		good_consumption = 1
	}

	TradeGood:new {
		name = "ingot-brass",
		description = "brass",
		icon = material_icon,
		r = r,
		g = g,
		b = b,
		base_price = 10,

		decay = 1
	}
	add_use_case("ingot-brass", "brass-source", 1)

	TradeGood:new {
		name = "jewelry-brass",
		description = "brass jewelry",
		icon = jewelry_icon,
		r = r,
		g = g,
		b = b,
		base_price = 10,

		decay = 1
	}
	add_use_case("jewelry-brass", "jewelry", 1)
	add_use_case("jewelry-brass", "gold-source", 1)

	---producing alloy

	ProductionMethod:new {
		name = "smelting-brass", description = "smelting copper and zinc into alloy",
		icon = material_icon, r = r, g = g,	b = b,
		inputs = { [use("tools")] = 0.25, [use("copper-source")] = 1, [use("structural-material")] = 0.1, [use("fuel")] = 5 },
		outputs = { [good("ingot-brass")] = 1 },
		job = job("smelters"), job_type = JOBTYPE.ARTISAN,
	}
	BuildingType:new {
		name = 'smelter-brass', description = 'smelter dedicated for brass',
		icon = material_icon, r = r, g = g, b = b,
		unlocked_by = tec("brass"),
		production_method = prod('smelting-brass'),
		needed_infrastructure = 2, construction_cost = 80,
		required_biome = {}, required_resource = {res("arsenic")},
	}

	---producing jewelry out of alloy

	ProductionMethod:new {
		name = "smith-jewelry-brass", description = "making brass jewelry",
		icon = material_icon, r = r, g = g,	b = b,
		inputs = { [use("tools")] = 0.25, [use("brass-source")] = 1 },
		outputs = { [good("jewelry-brass")] = 1 },
		job = job("blacksmiths"), job_type = JOBTYPE.ARTISAN,
	}
	BuildingType:new {
		name = 'smith-jewelry-brass', description = 'brass jewelry smith',
		icon = material_icon, r = r, g = g, b = b,
		unlocked_by = tec("brass"),
		production_method = prod('smith-jewelry-brass'),
		needed_infrastructure = 2, construction_cost = 80,
		required_biome = {}, required_resource = {},
	}
end