local p = {}
local ut = require "game.climate.utils"

function p.run()
	local function apply_from_closure(get_closure, set_closure)
		---@type climate_cell_id
		for cell = 0, CLIMATE_CELL.size() do
			local x, y = ut.get_x_y(cell)
			local l = ut.get_x(x - 1)
			local r = ut.get_x(x + 1)
			local u = ut.get_y(y + 1)
			local d = ut.get_y(y - 1)

			CLIMATE_CELL.set_cache(cell, 1,
				(
					get_closure(ut.get_id(l, y))
					+ get_closure(ut.get_id(r, y))
					+ get_closure(ut.get_id(x, u))
					+ get_closure(ut.get_id(x, d))
					+ get_closure(ut.get_id(x, y))
				) / 5.0
			)
		end

		---@type climate_cell_id
		for cell = 0, CLIMATE_CELL.size() do
			set_closure(cell, CLIMATE_CELL.get_cache(cell, 1))
		end
	end

	-- Smooth most factors...
	for _ = 1, 6 do
		apply_from_closure(
			CLIMATE_CELL.get_true_continentality,
			CLIMATE_CELL.set_true_continentality
		)
		apply_from_closure(
			CLIMATE_CELL.get_true_rain_shadow,
			CLIMATE_CELL.set_true_rain_shadow
		)
		apply_from_closure(
			CLIMATE_CELL.get_med_influence,
			CLIMATE_CELL.set_med_influence
		)
	end

	-- Adjust Hadley for continentality...
	-- Smooth hadley

	---@type climate_cell_id
	for cell = 0, CLIMATE_CELL.size() do
		local cont = CLIMATE_CELL.get_left_to_right_continentality(cell)
		local had = CLIMATE_CELL.get_hadley_influence(cell)
		had = had * (1 - math.min(1, math.max(0, cont / 0.025)))
		had = math.sqrt(had)

		CLIMATE_CELL.set_hadley_influence(cell, had)
	end
	for _ = 1, 6 do
		apply_from_closure(
			CLIMATE_CELL.get_hadley_influence,
			CLIMATE_CELL.set_hadley_influence
		)
	end
end

return p
