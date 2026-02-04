local realm_utils = require "game.entities.realm".Realm
local cult = require "game.entities.culture"
local rel = require "game.entities.religion"
local pop_utils = require "game.entities.pop".POP
local language_utils = require "game.entities.language".Language
local tabb = require "engine.table"
local tile      = require "game.entities.tile"

local tec = require "game.raws.raws-utils".technology

local politics_values = require "game.raws.values.politics"

local province_utils = require "game.entities.province".Province
local pe = require "game.raws.effects.politics"

local st = {}

require "codegen-lua.race"
require "codegen-lua.faith"
require "codegen-lua.realm"
require "codegen-lua.culture"


---Makes a new realm, one settlement large.
---@param capitol_id settlement_id
---@param race_id race_id
---@param culture culture_id
---@param faith faith_id
local function make_new_realm(capitol_id, race_id, culture, faith)
	-- print("new realm")

	local r = REALM.create();
	REALM.set_capitol(r, capitol_id)
	realm_utils.add_province(r, capitol_id)

	-- fat.name = language_utils.get_random_realm_name(DATA.culture_get_language(culture))

	REALM.set_primary_race(r, race_id)
	REALM.set_primary_culture(r, culture)
	REALM.set_primary_faith(r, faith)

	local males_per_hundred = RACE.get_males_per_hundred_females(race_id)
	local males_ratio = males_per_hundred / (100 + males_per_hundred)
	local fecundity = RACE.get_fecundity(race_id)

	-- We also need to spawn in some population...
	local pop_to_spawn = 10 * fecundity

	for random_number_source = 1, pop_to_spawn do
		local random_number = random_number_source / pop_to_spawn

		local age = random_number * (RACE.get_adult_age(race_id) - RACE.get_child_age(race_id)) + RACE.get_child_age(race_id)

		local new_pop = pop_utils.new(
			race_id,
			faith,
			culture,
			(random_number * 10) % 1 > males_ratio,
			-age,
			(random_number * 7) % 1 * WORLD.ticks_per_year
		)

		province_utils.add_pop(capitol_id, new_pop)
		province_utils.set_home(capitol_id, new_pop)
	end

	-- spawn leader

	do
		local elite_character = pe.generate_new_noble(r, capitol_id, race_id, faith, culture)
		local popularity = DATA.force_create_popularity(elite_character, r)
		local fat_popularity = DATA.fatten_popularity(popularity)
		fat_popularity.value = 50
		pe.transfer_power(r, elite_character, POLITICS_REASON.INITIALRULER)
	end

	-- spawn nobles
	for i = 1, pop_to_spawn / 10 do
		local contender = pe.generate_new_noble(r, capitol_id, race_id, faith, culture)
		local popularity = DATA.force_create_popularity(contender, r)
		local fat_popularity = DATA.fatten_popularity(popularity)
		fat_popularity.value = AGE_YEARS(contender) / 15
	end

	-- set up capitol
	capitol.name = language_utils.get_random_province_name(DATA.culture_get_language(culture))
	province_utils.research(capitol_id, tec('paleolithic-knowledge')) -- initialize technology...

	-- give some stuff to capitol
	capitol.infrastructure = love.math.random() * 10 + 10
	capitol.local_wealth = love.math.random() * 10 + 10
	capitol.trade_wealth = love.math.random() * 10 + 10

	-- give initial research budget
	DATA.realm_set_budget_budget(r, BUDGET_CATEGORY.EDUCATION, 1)

	-- starting treasury
	DATA.realm_set_budget_budget(r, BUDGET_CATEGORY.EDUCATION, 1)
	fat.budget_treasury = love.math.random() * 20 + 20 * pop_to_spawn

	-- give some realms early tech advantage to reduce waiting:
	for i = 0, 2 do
		---@type technology_id[]
		local to_research = {}
		DATA.for_each_technology(function (item)
			if DATA.province_get_technologies_researchable(capitol_id, item) == 1 then
				if love.math.random() < 0.1 then
					DATA.realm_inc_budget_budget(r, BUDGET_CATEGORY.EDUCATION, 1)
					table.insert(to_research, item)
				end
			end
		end)

		for _, item in pairs(to_research) do
			province_utils.research(capitol_id, item)
		end
	end

	-- match children pop to some possible parent
	DATA.for_each_pop_location_from_location(capitol_id, function (item)
		local child = DATA.pop_location_get_pop(item)
		local child_age = AGE_YEARS(child)

		if child_age > race.adult_age then
			return
		end
		local child_rank = IS_CHARACTER(child)

		---@type pop_id[]
		local parents = {}

		DATA.for_each_pop_location_from_location(capitol_id, function (parent_location)
			local potential_parent_id = DATA.pop_location_get_pop(parent_location)
			local age = AGE_YEARS(potential_parent_id)
			local rank = IS_CHARACTER(potential_parent_id)
			if rank ~= child_rank then
				return
			elseif age <= child_age + race.adult_age then
				return
			elseif age >= child_age + race.elder_age then
				return
			end
			table.insert(parents, potential_parent_id)
		end)

		local parent = tabb.random_select_from_array(parents)

		if parent then
			DATA.force_create_parent_child_relation(parent, child)
		end
	end)

	-- capitol:validate_population()

	-- print("test battle")
	-- local size_1, size_2 = love.math.random(50) + 10, love.math.random(50) + 10
	-- local army_1 = generate_test_army(size_1, race, faith, culture, capitol)
	-- local army_2 = generate_test_army(size_2, race, faith, culture, capitol)

	-- print(size_2, size_1)
	-- local victory, losses, def_losses = army_2:attack(capitol, true, army_1)
	-- print(victory, losses, def_losses)
end

require "codegen-lua.settlement_tile"
---Checks if province is eligible for spawn
---@param race race_id
---@param tile tile_id
---@return boolean
function TileCheck(race, tile)
	local settlement_rel = TILE.get_settlement_tile_as_tile(tile)
	local settlement = SETTLEMENT_TILE.get_settlement(settlement_rel)

	if settlement ~= INVALID_ID then
		return false;
	end

	if not TILE.get_is_land(tile) then
		return false;
	end

	if RACE.get_requires_large_river(race) then
		if TILE.get_july_waterflow(tile) + TILE.get_january_waterflow(tile) < 3000 then
			return false
		end
	end

	if RACE.get_requires_large_forest(race) then
		if TILE.get_conifer(tile) + TILE.get_broadleaf(tile) < 0.5 then
			return false
		end
	end

	local _, ja_t, _, ju_t = tile.get_climate_data(tile)
	local elevation = TILE.get_elevation(tile)
	local min_t = math.min(ja_t, ju_t)
	local avg_t = (ja_t + ju_t) / 2

	if RACE.get_minimum_comfortable_temperature(race) > avg_t then return false end
	if RACE.get_minimum_absolute_temperature(race) > min_t then return false end
	if RACE.get_minimum_comfortable_elevation(race) < elevation then return false end

	return true
end


---Spawns initial tribes and initializes their data (such as characters, cultures, religions, races, etc)
function st.run()
	---@type Queue<Province>
	local queue = require "engine.queue":new()


	-- order:
	-- river specialists races first
	-- forest specialists races second
	-- rest races at the end

	print("Decide spawn order for races")

	---@type Race[]
	local order = {}
	for _, r in pairs(RAWS_MANAGER.races_by_name) do
		if DATA.race_get_requires_large_river(r) then
			table.insert(order, r)
		end
	end

	for _, r in pairs(RAWS_MANAGER.races_by_name) do
		if DATA.race_get_requires_large_forest(r) and not DATA.race_get_requires_large_river(r) then
			table.insert(order, r)
		end
	end

	for _, r in pairs(RAWS_MANAGER.races_by_name) do
		if (not DATA.race_get_requires_large_forest(r)) and (not DATA.race_get_requires_large_river(r)) then
			table.insert(order, r)
		end
	end

	local civs = 500 / tabb.size(order) -- one per race...


	---@type table<culture_id, province_id[]>
	local provinces_per_cultures = {}

	print("Spawn starting races")

	-- print(civs)
	local random = 0
	for _ = 1, civs do
		for _, r in ipairs(order) do
			random = random + 1
			-- print("spawn" .. DATA.race_get_name(r))
			-- First, find a land province that isn't owned by any realm...
			local sampled_tile = WORLD:random_tile()
			while not TileCheck(r, sampled_tile) do
				sampled_tile = WORLD:random_tile()
			end

			-- An unowned province -- it means we can spawn a new realm here!
			local cg = cult.CultureGroup:new()
			local culture = cult.Culture:new(cg)

			CULTURE.set_traditional_militarization(culture, 0.05)

			local rg = rel.Religion:new(culture)
			local faith = rel.Faith:new(rg, culture)
			FAITH.set_burial_rites(faith, tabb.select_one((random % 10) / 10, {
				{
					weight = 1,
					entry = BURIAL_RIGHTS.BURIAL
				},
				{
					weight = 0.8,
					entry = BURIAL_RIGHTS.CREMATION
				},
				{
					weight = 0.2,
					entry = BURIAL_RIGHTS.NONE
				}
			}))
			make_new_realm(prov, r, culture, faith)
			queue:enqueue(prov)
		end
	end

	print("Flood fill the rest of the world")
	-- Loop through all entries in the queue and flood fill out
	while queue:length() > 0 do
		---@type Province
		local prov = queue:dequeue()
		local fat_prov = DATA.fatten_province(prov)
		local realm = province_utils.realm(prov)
		local culture = DATA.realm_get_primary_culture(realm)
		local race = DATA.realm_get_primary_race(realm)
		local faith = DATA.realm_get_primary_faith(realm)

		if provinces_per_cultures[culture] == nil then
			provinces_per_cultures[culture] = {}
		end

		table.insert(provinces_per_cultures[culture], prov)

		-- First, check for rng based on movement cost.
		-- This will make it so culture "expand" slowly through mountains and such.
		if (love.math.random() > 0.001 + fat_prov.movement_cost / 1000.0) or fat_prov.on_a_river then
			DATA.for_each_province_neighborhood_from_origin(prov, function (item)
				local neigh = DATA.province_neighborhood_get_target(item)
				local fat_neigh = DATA.fatten_province(neigh)
				local neigh_realm = province_utils.realm(neigh)

				local river_bonus = 1
				if fat_prov.on_a_river and fat_neigh.on_a_river then
					river_bonus = 0.25
				end
				if DATA.race_get_requires_large_river(race) then
					if fat_neigh.on_a_river then
						river_bonus = 0.001
					else
						river_bonus = 1000
					end
				end
				if (love.math.random() > 0.001 + fat_neigh.movement_cost / 1000.0 * river_bonus) then
					if TILE.get_is_land(fat_neigh.center) == TILE.get_is_land(fat_prov.center)
						and neigh_realm == INVALID_ID
						and fat_neigh.foragers_limit > 8
					then -- formerly 5.5
						-- We can spawn a new realm in this province! It's unused!
						make_new_realm(
							neigh,
							race,
							culture,
							faith
						)
						queue:enqueue(neigh)
					end
				end
			end)
		else
			-- queue:enqueue(prov)
		end
	end

	--- recalculate dbm weights

	for culture, provs in pairs(provinces_per_cultures) do
		---@type table<FORAGE_RESOURCE, number>
		local total_weights = {}
		local total_population = 0
		DATA.for_each_forage_resource(function (i)
			total_weights[i] = 0
		end)

		for _, prov in pairs(provs) do
			local province_dbm_weights = require "game.economy.diet-breadth-model".cultural_foragable_targets(prov)
			local local_population = province_utils.local_population(prov)
			DATA.for_each_forage_resource(function (i)
				total_weights[i] = total_weights[i] + province_dbm_weights[i] * local_population
			end)
			total_population = total_population + local_population
		end

		DATA.for_each_forage_resource(function (i)
			total_weights[i] = total_weights[i] / total_population
			DATA.culture_set_traditional_forager_targets(culture, i, total_weights[i])
		end)
	end

	local realms = 0
	DATA.for_each_realm(function (item)
		realms = realms + 1
	end)

	-- At the end, print the amount of spawned tribes
	print("Spawned tribes:", realms)
	local pops = 0
	local characters = 0
	DATA.for_each_province(function (item)
		pops = pops + province_utils.local_population(item)
		characters = characters + province_utils.local_characters(item)
	end)
	print("Spawned population: " .. tostring(pops))
	print("Spawned characters: " .. tostring(characters))
end

return st
