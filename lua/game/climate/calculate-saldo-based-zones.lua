local sa = {}

---Applies a single variable
---@param center number
---@param thickness number
---@param saldo_influence number
---@param mirror boolean
---@param apply_func function
local function apply_saldo_based_variable(center, thickness, saldo_influence, mirror, apply_func)
	local ut = require "game.climate.utils"
	local cells_per_degree = WORLD.climate_grid_size / 180.0

	---@type climate_cell_id
	for cell = 0, CLIMATE_CELL.size() - 1 do
		local saldo_n = CLIMATE_CELL.get_saldo_north(cell)
		local saldo_s = CLIMATE_CELL.get_saldo_south(cell)
		local cont = saldo_n - saldo_s
		cont = math.min(0.22, math.max(-0.22, cont))
		cont = cont / 0.22

		local dist = cont * cells_per_degree * saldo_influence

		-- Calculate centers of the zones.
		-- We need to do this, because the center gets skewed by distribution of land within the world.
		local new_center = WORLD.climate_grid_size / 2 + center * cells_per_degree + dist
		local mirr_center = WORLD.climate_grid_size / 2 - center * cells_per_degree + dist

		local _, y = ut.get_x_y(cell)
		y = y + 0.5

		if y > new_center - thickness and y < new_center + thickness then
			local influence = 1 - math.abs(new_center - y) / thickness
			apply_func(cell, influence)
		end
		if mirror then
			if y > mirr_center - thickness and y < mirr_center + thickness then
				local influence = 1 - math.abs(mirr_center - y) / thickness
				apply_func(cell, influence)
			end
		end
	end
end

function sa.run()
	-- HADLEY
	apply_saldo_based_variable(24, 10, 6, true, CLIMATE_CELL.set_hadley_influence)
	-- ITCZ
	apply_saldo_based_variable(-8, 15, 3, false, CLIMATE_CELL.set_itcz_january)
	apply_saldo_based_variable(8, 15, -3, false, CLIMATE_CELL.set_itcz_july)
	---@type climate_cell_id
	for cell = 0, CLIMATE_CELL.size() - 1 do
		local dist = CLIMATE_CELL.get_distance_to_sea(cell)
		local dist_factor = math.min(WORLD.climate_grid_size, dist / 0.1) / WORLD.climate_grid_size

		local itcz_winter = CLIMATE_CELL.get_itcz_january(cell)
		local itcz_summer = CLIMATE_CELL.get_itcz_july(cell)

		CLIMATE_CELL.set_itcz_january(cell, itcz_winter * (1 - dist_factor))
		CLIMATE_CELL.set_itcz_july(cell, itcz_summer * (1 - dist_factor))
	end
	-- MED
	apply_saldo_based_variable(32, 8, 4, true, CLIMATE_CELL.set_med_influence)
	---@type climate_cell_id
	for cell = 0, CLIMATE_CELL.size() - 1 do
		local dist = CLIMATE_CELL.get_distance_to_sea(cell)
		local dist_factor = math.min(WORLD.climate_grid_size, dist / 0.05) / WORLD.climate_grid_size
		local med = CLIMATE_CELL.get_med_influence(cell)
		CLIMATE_CELL.set_med_influence(cell, med  * (1 - dist_factor))
	end
end

return sa
