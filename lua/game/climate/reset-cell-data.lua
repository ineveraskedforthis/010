local r = {}

local tile= require "game.entities.tile"

local ut = require "game.climate.utils"

function r.run()
	for i = 0, TILE.size() - 1 do
		WORLD.tile_to_climate_cell[i] = ut.get_climate_cell(tile.latlon(i))
	end
end

function r.run_hex(world)
	world:for_each_tile(function(i, _)
		local lat, lon = world:get_latlon_by_tile(i)
		world.climate_cells[i + 1] = ut.get_climate_cell(lat, lon)
	end)
end

return r
