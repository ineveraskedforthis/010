local cl = {}

local function update_climate_cells()
	---@type climate_cell_id
	for cell = 0, CLIMATE_CELL.size() - 1 do
		local land = CLIMATE_CELL.get_land_tiles(cell)
		local water =  CLIMATE_CELL.get_water_tiles(cell)
		local tt = land + water
		if tt > 0 then
			CLIMATE_CELL.set_elevation(cell, CLIMATE_CELL.get_elevation(cell) / tt)
			CLIMATE_CELL.set_water_fraction(cell, water / tt)
		else
			CLIMATE_CELL.set_elevation(cell, 0);
			CLIMATE_CELL.set_water_fraction(cell, 1);
			CLIMATE_CELL.set_water_tiles(cell, 1);
		end
	end
end

function cl.run()
	for tile_id, cell in pairs(WORLD.tile_to_climate_cell) do
		local elevation = TILE.get_elevation(tile_id)
		local total_elevation = CLIMATE_CELL.get_elevation(cell)
		CLIMATE_CELL.set_elevation(cell, total_elevation + elevation)

		local land_tiles = CLIMATE_CELL.get_land_tiles(cell)
		local water_tiles = CLIMATE_CELL.get_water_tiles(cell)

		if TILE.get_is_land(tile_id) then
			CLIMATE_CELL.set_land_tiles(cell, land_tiles + 1)
		else
			CLIMATE_CELL.set_water_tiles(cell, water_tiles + 1)
		end
	end

	update_climate_cells()
end

function cl.run_hex(world)
	world:for_each_tile(function(i, _)
		local cell = world.climate_cells[i + 1]

		cell.elevation = world.elevation[i]

		local is_land = world.is_land[i]
		if is_land then
			cell.land_tiles = cell.land_tiles + 1
		else
			cell.water_tiles = cell.water_tiles + 1
		end
	end)

	update_climate_cells()
end

return cl
