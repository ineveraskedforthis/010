local wl = {}


local world = require "game.entities.world"
local plate = require "game.entities.plate"
local color = require "game.color"
local tabb = require "engine.table"
local tile = require "game.entities.tile"
local method_utils = require "game.raws.production-methods"
require "codegen-lua.tile"

WORLD_PROGRESS = {total = 0, max = 0, is_loading = false}

local loader_error = nil -- write this in coroutines to transmit the error out of coroutines scope...
---
function wl.init()
	DEFINES = require "game.defines".init()
	local tips = require "game.scenes.tips"
	local size = tabb.size(tips)
	print("Tip table size: " .. tostring(size))
	local r = 1
	print("Randomly rolled tip index: " .. tostring(r))
	wl.tip = tips[r]
	wl.load_default()
end

---
---@param dt number
function wl.update(dt)

end

---
function wl.draw()

	ui.background(ASSETS.background)

	if wl.coroutine == nil then
		wl.message = "Initializing..."
		if DEFINES.empty then
			wl.coroutine = coroutine.create(wl.empty)
		elseif DEFINES.default then --(require "engine.table").contains(ARGS, "--dev") then
			-- We're loading a world from default pngs for debugging purposes...
			wl.coroutine = coroutine.create()
		elseif DEFINES.world_gen then
			-- We're generating a world from scratch...
			wl.coroutine = coroutine.create(wl.generate)
		else
			-- We're loading a world from file...
			wl.coroutine = coroutine.create(wl.load_save)
		end
	end
	local output = {coroutine.resume(wl.coroutine)}

	ui.text_panel(wl.message, ui.fullscreen():subrect(
		0, 0, 300, 60, "center", "down"
	))
	ui.text_panel(wl.tip, ui.fullscreen():subrect(
		0, 0, 800, 60, "center", "up"
	))

	if coroutine.status(wl.coroutine) == "dead" then
		-- Well, if the coroutine is dead it means that loading finished...
		-- print(output[2])
		-- print(debug.traceback(wl.coroutine))
		if loader_error ~= nil then
			error(loader_error)
			return
		end
		wl.coroutine = nil
		local manager = require "game.scene-manager"
		manager.transition("game")
	end
end

---Given a tile ID and an image data, return the color for that tile
---@param tile_id tile_id
---@param map love.ImageData
---@return number r
---@return number g
---@return number b
local function read_pixel(tile_id, map)
	local lat, lon = tile.latlon(tile_id)
	local y = (lat + math.pi / 2) / math.pi
	y = math.min(1, math.max(0, y)) * map:getHeight()
	y = math.min(map:getHeight() - 1, math.max(0, y))
	local x = (lon + math.pi) / (2 * math.pi)
	x = math.min(1, math.max(0, x)) * map:getWidth()
	x = math.min(map:getWidth() - 1, math.max(0, x))
	local r, g, b, _ = map:getPixel(x, y)
	return r, g, b
end

function wl.empty()
	print("Loading an empty world...")

	wl.message = "Loading an empty world..."

	world.empty()



	require "game.raws.raws" ()



end

function wl.load_default()
	print("Loading default world...")
	wl.message = "Loading default world..."


	do
		print("Generating climate...")
		CLIMATE_CELL.resize_cache(10)
		require "game.climate.climate-simulation".run()
		for i = 0, TILE.size() - 1 do
			tile.update_climate_data(i)
		end
		print("Climate generated!")
	end

	do
		print("Generating plants...")
		require "game.ecology.plant-simulation".run()
		print("Plants generated!")
	end


	do
		print("Generating biomes...")
		require "game.ecology.recalculate-biomes".run_fast()
		print("Biomes generated!")
	end

	do
		print("Generating resources...")
		require "game.world-gen.resource-gen".run()
		print("Resources generated!")
	end

	do
		print("Calculating pathfinding indices")
		require "game.world-gen.determine-pathfinding-index".determine()
		print("Pathfinding indices calculated!")
	end
end

function wl.generate()






	wl.message = "Generating..."






	world.empty()



	require "game.raws.raws" ()





	wl.message = "Generating climate..."


	do
		local time = love.timer.getTime()
		require "game.climate.climate-simulation".run()
		print(love.timer.getTime() - time)
	end



	wl.message = "Generating plants..."


	do
		local time = love.timer.getTime()
		require "game.ecology.plant-simulation".run()
		print(love.timer.getTime() - time)
	end



	wl.message = "Generating biomes..."


	do
		local time = love.timer.getTime()
		require "game.ecology.recalculate-biomes".run()
		print(love.timer.getTime() - time)
	end



	wl.message = "Calculating pathfinding indices"


	do
		local time = love.timer.getTime()
		require "game.world-gen.determine-pathfinding-index".determine()
		print(love.timer.getTime() - time)
	end



	wl.message = "Generating resources..."


	do
		local time = love.timer.getTime()
		require "game.world-gen.resource-gen".run()
		print(love.timer.getTime() - time)
	end
end

function wl.load_save()






	wl.message = "Loading save..."






	DCON.dcon_reset()
	world.empty()
	-- print('loading raws')
	-- require "game.raws.raws" ()
	print("Load game state")
	LOAD_GAME_STATE()
	assert(WORLD)
	require "game.entities.world".reset_metatable(WORLD)
	require "game.raws.raws"(true, true)

	-- trasfer world time to backend
	DCON.set_world_tick_definitions(WORLD.ticks_per_minute, WORLD.ticks_per_hour, WORLD.ticks_per_day, WORLD.ticks_per_month)
	print(DCON.get_world_ticks_per_minute(),DCON.get_world_ticks_per_hour(),DCON.get_world_ticks_per_day(),DCON.get_world_ticks_per_month())
	DCON.set_world_current_tick(WORLD.current_tick_in_year)
	DCON.set_world_current_year(WORLD.year)

	print("loading options")
	OPTIONS = require "game.options".load()
	require "game.options".verify()
	WORLD_PROGRESS.is_loading = false

	if WORLD == nil then
		return nil
	else
		loader_error = nil
	end
end

return wl
