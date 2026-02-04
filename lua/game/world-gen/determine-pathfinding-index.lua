local tile = require "game.entities.tile"
local dpi = {}

function dpi.determine()
	-- First, clear old indices
	for tile_id = 0, TILE.size() - 1 do
		TILE.set_pathfinding_index(tile_id, 0)
	end

	-- then, flood fill to fill new indices!
	---@type Queue<tile_id>
	local queue = require "engine.queue":new()
	local index = 0
	for tile_id = 0, TILE.size() - 1 do
		if TILE.get_pathfinding_index(tile_id) == 0 then
			-- unasigned tile! time to flood fill!
			index = index + 1
			TILE.set_pathfinding_index(tile_id, index)
			queue:enqueue(tile_id)
			while queue:length() > 0 do
				local pt = queue:dequeue()
				for neigh in tile.iter_neighbors(pt) do
					if TILE.get_pathfinding_index(neigh) == 0 and TILE.get_is_land(neigh) == TILE.get_is_land(tile_id) then
						TILE.set_pathfinding_index(neigh, index)
						queue:enqueue(neigh)
					end
				end
			end
		end
	end
end

return dpi
