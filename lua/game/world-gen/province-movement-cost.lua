local pro = {}

-- After provinces are created, we also need to create the neighborhoods and travel time costs
---@param t tile_id
function pro.single_tile_cost(t)
	if TILE.get_is_land(t) then
		local elevation = TILE.get_elevation(t)
		local grass = TILE.get_grass(t)
		local shrub = TILE.get_shrub(t)
		local conifer = TILE.get_shrub(t)
		local broadleaf = TILE.get_shrub(t)

		local elevation_cost = math.max(elevation, 0) / 1000.0
		local plant_cost = 0.01 * grass + 0.1 * shrub + 1 * conifer + 2 * broadleaf
		local ice_cost = 0
		if TILE.get_ice(t) > 0 then
			ice_cost = 10
		end
		return elevation_cost + plant_cost + ice_cost
	else
		if TILE.get_ice(t) > 0 then
			return 50
		else
			return 1
		end
	end
end

function pro.run()
	DATA.for_each_province(function (province_id)
		local fat = DATA.fatten_province(province_id)
		fat.movement_cost = 0
		for _, tile_member in pairs(DATA.get_tile_province_membership_from_province(province_id)) do
			fat.movement_cost = fat.movement_cost + pro.single_tile_cost(TILE.province_membership_get_tile(tile_member))
		end
	end)
end

return pro
