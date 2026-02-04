require "codegen-lua.building_type"

local d = {}

BUILDING_ARCHETYPE = {
	GROUND = 0,
	MINE = 1,
	HOUSE = 2
}

function d.load()
	local BuildingType = require "game.raws.building-types"

	BuildingType:new {
		name = "Grounds",
		description = "Area dedicated for certain activity",
		icon = "hut.png",
		r = 0,
		g = 1,
		b = 1,
		construction_cost = 50,
		needed_infrastructure = 1,
		ai_weight = 1,
		required_resource = {},
		required_biome = {},
		archetype = BUILDING_ARCHETYPE.GROUND
	}
	BuildingType:new {
		name = "House",
		description = "Primitive hut",
		icon = 'meat.png',
		r = 1.0,
		g = 0.2,
		b = 0.3,
		construction_cost = 100,
		needed_infrastructure = 1,
		ai_weight = 1,
		required_resource = {},
		required_biome = {},
		archetype = BUILDING_ARCHETYPE.HOUSE
	}
	BuildingType:new {
		name = "Mine",
		description = "hunting grounds",
		icon = 'stone-spear.png',
		r = 1.0,
		g = 0.2,
		b = 0.3,
		construction_cost = 300,
		needed_infrastructure = 1,
		ai_weight = 1,
		required_resource = {},
		required_biome = {},
		archetype = BUILDING_ARCHETYPE.MINE
	}
end

return d
