local so = {}

local ut = require "game.map-modes.utils"
local tile = require "game.entities.tile"


function so.texture()
	DATA.for_each_tile(function (tile_id)
		ut.set_default_color(tile_id)

		local depth = tile.soil_depth(tile_id)
		local sand = TILE.get_sand(tile_id)
		local silt = TILE.get_silt(tile_id)
		local clay = TILE.get_clay(tile_id)

		tile.set_real_color(tile_id,
			sand / depth,
			silt / depth,
			clay / depth
		)
	end)
end

function so.depth()
	ut.simple_hue_map_mode(function(tile_id)
		return math.min(1, tile.soil_depth(tile_id) / 25)
	end)
end

function so.organics()
	ut.simple_hue_map_mode(function(tile_id)
		return math.min(1, TILE.get_soil_organics(tile_id))
	end)
end

function so.minerals()
	ut.simple_hue_map_mode(function(tile_id)
		return math.min(1, TILE.get_soil_minerals(tile_id))
	end)
end

return so
