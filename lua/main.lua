local ffi = require "ffi"

PATH_TO_ASSETS_FOLDER = "./lua/icons/"
INVALID_ID = -1

ffi.cdef[[
	void* calloc( size_t num, size_t size );
	void update_vegetation(float);
	void update_economy();

	void apply_biome(int32_t);
	void apply_resource(int32_t);

	float estimate_province_use_price(uint32_t, uint32_t);
	float estimate_building_type_income(int32_t, int32_t, int32_t, bool);
	void dcon_everything_write_file(char const* name);
	void dcon_everything_read_file(char const* name);
	void update_foraging_data(
		int32_t province_raw_id,
		int32_t water_raw_id,
		int32_t berries_raw_id,
		int32_t grain_raw_id,
		int32_t bark_raw_id,
		int32_t timber_raw_id,
		int32_t meat_raw_id,
		int32_t hide_raw_id,
		int32_t mushroom_raw_id,
		int32_t shellfish_raw_id,
		int32_t seaweed_raw_id,
		int32_t fish_raw_id,
		int32_t world_size
	);

	void load_state(char const*);
	int32_t dcon_reset();

	void update_map_mode_pointer(uint8_t* map, uint32_t world_size);

	void ai_update_price_belief(int32_t trader_raw_id);
	void ai_trade(int32_t trader_raw_id);

	// backend time tracking
	void set_world_current_year(uint32_t year);
	uint32_t get_world_current_year(void);
	void set_world_current_tick(uint32_t tick);
	uint32_t get_world_current_tick(void);
	void set_world_tick_definitions(uint32_t minute, uint32_t hour, uint32_t day, uint32_t month);
	uint32_t get_world_ticks_per_minute(void);
	uint32_t get_world_ticks_per_hour(void);
	uint32_t get_world_ticks_per_day(void);
	uint32_t get_world_ticks_per_month(void);
	// birthdate values
	uint32_t birth_month(uint32_t pop_id);
	uint32_t birth_day(uint32_t pop_id);
	uint32_t birth_hour(uint32_t pop_id);
	uint32_t birth_minute(uint32_t pop_id);
	// age calculations
	uint32_t age_ticks(uint32_t pop_id);
	uint32_t age_months(uint32_t pop_id);
	uint32_t age_years(uint32_t pop_id);
	float age_multiplier(uint32_t pop_id);
	float job_efficiency(uint32_t,uint8_t);
    // pop time calculations
	float pop_free_time(uint32_t pop);
	float pop_warband_time(uint32_t pop,float free);
	float pop_forage_time(uint32_t pop,float free,float party);
	float pop_work_time(uint32_t pop,float free,float party,float forage);
	// misc
	bool pop_same_location(uint32_t pop_a,uint32_t pop_b);
	bool is_dependent(uint32_t child);
	bool is_dependent_of(uint32_t child,uint32_t parent);
	uint32_t register_text(int32_t text_len, const char* data);
	uint32_t register_texture(int32_t text_len, const char* data);

	void change_scene(uint8_t scene);
]]

if arg and arg[#arg] == "-debug" then
	require("mobdebug").start()
	require("mobdebug").coro()
end

--- A table containing the passed arguments.
ARGS = {} -- note, hot loading won't overwrite ARGS because the declaration is empty
-- A table containing some basic asset references.
ASSETS = {}
-- A version string, kinda irrelevant now since multiplayer isn't a thing, lol
VERSION_STRING = "v0.3.0 (Midgard)"

SILENT_ASSET_LOADING = false

--if WORLD == nil then
---@type World|nil
WORLD = nil
--end

---@type string
MONEY_SYMBOL = '§'

PROFILE_FLAG = false
---@type table
PROFILER = {}

PROFILER.total = 0

---@type table<string, number>
PROFILER.timers = {}

---@type table<string, number>
PROFILER.data = {}

---@type table<string, number>
PROFILER.mean = {}

---@type table<string, number>
PROFILER.count = {}

function PROFILER.start_timer(self, tag)
	if not PROFILE_FLAG then return end
	self.timers[tag] = love.timer.getTime()
end

function PROFILER.end_timer(self, tag)
	if not PROFILE_FLAG then return end

	local now = love.timer.getTime()
	local delta = now - self.timers[tag]

	if self.data[tag] == nil then
		self.data[tag] = 0
		self.count[tag] = 0
	end
	self.count[tag] = self.count[tag] + 1
	self.data[tag] = self.data[tag] + delta
	self.total = self.total + delta

	self.mean[tag] = self.data[tag] / self.count[tag]
end

function PROFILER.clear(self)
	for tag, value in pairs(self.data) do
		self.data[tag] = 0
		self.mean[tag] = 0
		self.count[tag] = 0
	end
end

--- this constant is used in vegetation growth
--- vegetation = old_vegetation * (1 - VEGETATION_GROWTH) + ideal_vegetation * VEGETATION_GROWTH
VEGETATION_GROWTH = 0.005
PRICE_SIGNAL_PER_UNIT = 0.1
PRICE_SIGNAL_PER_STOCKPILED_UNIT = 0.05
PRICE_DIFFUSION = 0.3

DISPLAY_INCOME_OWNER_RATIO = 0

EMPLOYMENT_YEARS = 2

--TODO GLOBALIZE
---@enum BURIAL_RITES
BURIAL_RIGHTS = {
	INVALID = 0,
	CREMATION = 1,
	BURIAL = 2,
	NONE = 3
}
BURIAL_NAMES = {
	[BURIAL_RIGHTS.INVALID] = "INVALID",
	[BURIAL_RIGHTS.CREMATION] = "CREMATION",
	[BURIAL_RIGHTS.BURIAL] = "BURIAL",
	[BURIAL_RIGHTS.NONE] = "NONE",
}

local bs = require "engine.bitser"
-- Extra classes
bs.registerClass('Queue', require "engine.queue")

-- bs.registerClass("BiogeographicRealm", require "game.raws.biogeographic-realms")
-- bs.registerClass("Biome", require "game.raws.biomes")
-- Entities
-- bs.registerClass("ClimateCell", require "game.entities.climate-cell".ClimateCell)
-- bs.registerClass("CultureGroup", require "game.entities.culture".CultureGroup)
-- bs.registerClass("Language", require "game.entities.language".Language)
-- bs.registerClass("Religion", require "game.entities.religion".Religion)
-- bs.registerClass("World", require "game.entities.world".World)

local lovetest = require "test.lovetest"

---@type table<trade_good_id, table<use_case_id, number>>
USE_WEIGHT = {}

function RECALCULATE_WEIGHTS_TABLE()
	require "codegen-lua.trade_good"
	require "codegen-lua.use_case"

	for trade_good = 0, TRADE_GOOD.size() - 1 do
		USE_WEIGHT[trade_good] = {}
		for use_case = 0, USE_CASE.size() - 1 do
			USE_WEIGHT[trade_good][use_case] = 0
		end
	end

	for use_weight = 0, USE_WEIGHT.size() - 1 do
		local good = USE_WEIGHT.get_trade_good(use_weight)
		local use = USE_WEIGHT.get_trade_good(use_weight)
		assert(
			good ~= INVALID_ID,
			tostring(use_weight) .. " " ..tostring(good)
		)
		assert(
			use ~= INVALID_ID,
			tostring(use_weight) ..tostring(use)
		)
		USE_WEIGHT[good][use] = USE_WEIGHT.get_weight(use_weight)
	end
end

local world = require "game.entities.world"

---@enum UNIT_TYPE
UNIT_TYPE = {
	INVALID = 0,
	WARRIOR = 1,
	CIVILIAN = 2,
	FOLLOWER = 3,
}

function sote.load_raws()
	DEFINES = require "game.defines".init()
	world.empty()
	require "game.raws.raws" ()
end

function sote.load_world()
	require("game.scene-manager").init()

	require("game.scene-manager").transition('world-loader')
end

function love.update(dt)
	if tab.contains(ARGS, "--dev") then
		-- http://127.0.0.1:8000 <- to view lovebird
		require("engine.lovebird").update()
	end
	require("game.scene-manager").update(dt)
	require("game.music").update()
end

function love.quit()
	print("Thanks for playing!")
	if GAME_STATE.scene[2] then
		if GAME_STATE.scene[2].paused ~= nil then
			GAME_STATE.scene[2].paused = true
		end
	end
end