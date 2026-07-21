// derived from Songs of FOSS

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <iostream>
#include "data.hpp"
#define DCON_LUADLL_EXPORTS
#include "sote_functions.hpp"
#include "sote_types.hpp"
#include "lua-export.cpp"

#ifdef _WIN32
#include <fileapi.h>
#include <WinBase.h>
#include <winnls.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

// void save_to_file() {
// 	state.make_s
// }


struct tile_cube_coord {
	int32_t x;
	int32_t y;
	int32_t f;
};

// backend time tracking
static uint32_t WORLD_CURRENT_YEAR;
static uint32_t WORLD_CURRENT_TICK;
static uint32_t WORLD_TICKS_PER_MINUTE;
static uint32_t WORLD_TICKS_PER_HOUR;
static uint32_t WORLD_TICKS_PER_DAY;
static uint32_t WORLD_TICKS_PER_MONTH;

void set_world_current_year(uint32_t year) {
	WORLD_CURRENT_YEAR = year;
}
uint32_t get_world_current_year(void) {
	return WORLD_CURRENT_YEAR;
}
void set_world_current_tick(uint32_t tick) {
	WORLD_CURRENT_TICK = tick;
}
uint32_t get_world_current_tick(void) {
	return WORLD_CURRENT_TICK;
}
void set_world_tick_definitions(uint32_t minute, uint32_t hour, uint32_t day, uint32_t month) {
	WORLD_TICKS_PER_MINUTE = minute;
	WORLD_TICKS_PER_HOUR = hour;
	WORLD_TICKS_PER_DAY = day;
	WORLD_TICKS_PER_MONTH = month;
}
uint32_t get_world_ticks_per_minute(void) {
	return WORLD_TICKS_PER_MINUTE;
}
uint32_t get_world_ticks_per_hour(void) {
	return WORLD_TICKS_PER_HOUR;
}
uint32_t get_world_ticks_per_day(void) {
	return WORLD_TICKS_PER_DAY;
}
uint32_t get_world_ticks_per_month(void) {
	return WORLD_TICKS_PER_MONTH;
}

// Given a tile ID, returns x/y/f coordinates.
tile_cube_coord id_to_coords(int32_t tile_id, uint32_t world_size) {
	auto adjusted_id = (double)(tile_id - 1);
	auto ws = (double)world_size;
	auto f = std::floor(adjusted_id / (ws * ws));
	auto remaining = adjusted_id - f * ws * ws;
	auto y = std::floor(remaining / ws);
	auto x = remaining - y * ws;
	return {
		(int32_t)x, (int32_t)y, (int32_t)f
	};
}

int32_t coords_to_id(int32_t x, int32_t y, int32_t f, uint32_t world_size) {
	return 1 + (x + y * world_size + f * world_size * world_size);
}

constexpr inline uint8_t NEIGH_TOP = 1;
constexpr inline uint8_t NEIGH_BOTTOM = 2;
constexpr inline uint8_t NEIGH_RIGHT = 3;
constexpr inline uint8_t NEIGH_LEFT = 4;

constexpr inline uint8_t cube_FRONT = 0;
constexpr inline uint8_t cube_LEFT = 1;
constexpr inline uint8_t cube_BACK = 2;
constexpr inline uint8_t cube_RIGHT = 3;
constexpr inline uint8_t cube_TOP = 4;
constexpr inline uint8_t cube_BOTTOM = 5;

static auto GOOD_CATEGORY = (uint8_t)((base_types::TRADE_GOOD_CATEGORY::GOOD));

constexpr inline float MAX_INDUCED_DEMAND = 3.f;

// how much of income is siphoned to local wealth pool
constexpr inline float INCOME_TO_LOCAL_WEALTH_MULTIPLIER = 0.125f / 4.f;

// pops work at least this time
constexpr inline float MINIMAL_WORKING_RATIO = 0.2f;

constexpr inline float spending_ratio = 0.1f;

float forage_efficiency(float foragers, float carrying_capacity) {
	if (foragers > carrying_capacity) {
		return carrying_capacity / (foragers + 1);
	} else {
		return 2 - expf(-0.7*(carrying_capacity - foragers)/carrying_capacity);
	}
}

void load_state(char const* name) {
#ifdef _WIN32
	int wchars_num = MultiByteToWideChar( CP_UTF8 , 0 , name , -1, NULL , 0 );
	wchar_t* w_name = new wchar_t[wchars_num];
	MultiByteToWideChar( CP_UTF8 , 0 , name , -1, w_name , wchars_num );

	auto file_handle = CreateFileW(
		w_name,
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
		nullptr
	);

	if(file_handle != INVALID_HANDLE_VALUE) {
		auto mapping_handle = CreateFileMappingW(file_handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
		if(mapping_handle) {
			auto data = (std::byte const*)MapViewOfFile(mapping_handle, FILE_MAP_READ, 0, 0, 0);
			if(data) {
				_LARGE_INTEGER pvalue;
				GetFileSizeEx(file_handle, &pvalue);
				auto file_size = uint32_t(pvalue.QuadPart);

				dcon::load_record loaded;
				dcon::load_record selection = state.make_serialize_record_everything();
				state.deserialize(data, data + file_size, loaded, selection);

				UnmapViewOfFile(data);
			}
			CloseHandle(mapping_handle);
		}
		CloseHandle(file_handle);
	}
	delete[] w_name;
#else
	int file_descriptor = open(name, O_RDONLY | O_NONBLOCK);
	if (file_descriptor != -1) {
		struct stat sb;
		if(fstat(file_descriptor, &sb) != -1) {
			auto file_size = sb.st_size;
#if _POSIX_C_SOURCE >= 200112L
			posix_fadvise(file_descriptor, 0, static_cast<off_t>(file_size), POSIX_FADV_WILLNEED);
#endif
#if defined(_GNU_SOURCE) || defined(_DEFAULT_SOURCE) || defined(_BSD_SOURCE) || defined(_SVID_SOURCE)
			void* mapping_handle = mmap(0, file_size, PROT_READ, MAP_PRIVATE, file_descriptor, 0);
			assert(mapping_handle != MAP_FAILED);
			std::byte const* content = static_cast<std::byte const*>(mapping_handle);
			dcon::load_record loaded;
			dcon::load_record selection = state.make_serialize_record_everything();
			state.deserialize(content, content + file_size, loaded, selection);
			if(munmap(mapping_handle, file_size) == -1) {
				assert(false);
			}
#else
			void* buffer = malloc(file_size);
			read(file_descriptor, buffer, file_size);
			std::byte const* content = static_cast<std::byte const*>(buffer);
			dcon::load_record loaded;
			dcon::load_record selection = state.make_serialize_record_everything();
			state.deserialize(content, content + file_size, loaded, selection);
			free(buffer);
#endif
		}
		close(file_descriptor);
	}
#endif
}

// converting birth tick into human readable values
uint32_t birth_month(dcon::pop_id pop) {
	auto birthtick = state.pop_get_birth_tick(pop);
	auto month = birthtick / WORLD_TICKS_PER_MONTH;
//	std::cout << std::to_string(month) + " = " + std::to_string(birthtick) + " / " + std::to_string(WORLD_TICKS_PER_MONTH) + "\n";
	return month;
}
uint32_t birth_day(dcon::pop_id pop) {
	auto birthtick = state.pop_get_birth_tick(pop);
	auto month = birthtick / WORLD_TICKS_PER_MONTH;
	auto day_tick = birthtick - month * WORLD_TICKS_PER_MONTH;
	auto day = day_tick / WORLD_TICKS_PER_DAY;
//	std::cout << std::to_string(day) + " = " + std::to_string(day_tick) + " / " + std::to_string(WORLD_TICKS_PER_DAY) + "\n";
	return day+1; // since day cycles between 1 and 30
}
uint32_t birth_hour(dcon::pop_id pop) {
	auto birthtick = state.pop_get_birth_tick(pop);
	auto month = birthtick / WORLD_TICKS_PER_MONTH;
	auto day_tick = birthtick - month * WORLD_TICKS_PER_MONTH;
	auto day = day_tick / WORLD_TICKS_PER_DAY;
	auto hour_tick = day_tick - day * WORLD_TICKS_PER_DAY;
	auto hour = hour_tick / WORLD_TICKS_PER_HOUR;
//	std::cout << std::to_string(hour) + " = " + std::to_string(hour_tick) + " / " + std::to_string(WORLD_TICKS_PER_HOUR) + "\n";
	return hour;
}
uint32_t birth_minute(dcon::pop_id pop) {
	auto birthtick = state.pop_get_birth_tick(pop);
	auto month = birthtick / WORLD_TICKS_PER_MONTH;
	auto day_tick = birthtick - month * WORLD_TICKS_PER_MONTH;
	auto day = day_tick / WORLD_TICKS_PER_DAY;
	auto hour_tick = day_tick - day * WORLD_TICKS_PER_DAY;
	auto hour = hour_tick / WORLD_TICKS_PER_HOUR;
	auto minute_tick = hour_tick - hour * WORLD_TICKS_PER_HOUR;
	auto minute = minute_tick / WORLD_TICKS_PER_MINUTE;
//	std::cout << std::to_string(minute) + " = " + std::to_string(minute_tick) + " / " + std::to_string(WORLD_TICKS_PER_MINUTE) + "\n";
	return minute;
}
// converting birth year and tick into age values
uint32_t age_ticks(dcon::pop_id pop) {
	return (WORLD_CURRENT_YEAR - state.pop_get_birth_year(pop))
		* WORLD_TICKS_PER_MONTH * 12 + WORLD_CURRENT_TICK - state.pop_get_birth_tick(pop);
}
uint32_t age_months(dcon::pop_id pop) {
	return age_ticks(pop) / WORLD_TICKS_PER_MONTH;
}
uint32_t age_years(dcon::pop_id pop) {
	return age_ticks(pop) / WORLD_TICKS_PER_MONTH / 12;
}
// using age values
float age_multiplier(dcon::pop_id pop) {
	float age_multiplier = 1.f;
	auto age = age_ticks(pop);
	auto race = state.pop_get_race(pop);

	auto conversion = WORLD_TICKS_PER_MONTH * 12;
	auto adult_age = state.race_get_adult_age(race) * conversion;
	auto middle_age = state.race_get_middle_age(race) * conversion;
	auto max_age = state.race_get_max_age(race) * conversion;

	if (age < adult_age) {
		age_multiplier = 0.25 + 0.75 * age / adult_age; // [.25,1.f)
	} else if (age >= middle_age) {
		age_multiplier = 1.f - 0.1 * (age - middle_age) / (max_age - middle_age); // [1.f,.75)
	}
	return age_multiplier;
}
// pop time calculations
float pop_free_time(dcon::pop_id pop) {
	auto age = age_ticks(pop);
	auto race = state.pop_get_race(pop);
	auto teen = state.race_get_teen_age(race) * WORLD_TICKS_PER_MONTH * 12;
	if (age < teen) {
		return age / teen;
	} else {
		return 1.f;
	}
}
float pop_warband_time(dcon::pop_id pop, float free) {
	auto remaining = free - 0.05f;
	if (remaining <= 0.f) {
		return 0.f;
	}
	auto unitship = state.pop_get_warband_unit_as_unit(pop);
	auto warband = state.warband_unit_get_warband(unitship);
	if (state.warband_is_valid(warband)) {
		auto time = state.warband_get_current_time_used_ratio(warband);
		if (remaining < time) {
			return remaining;
		} else {
			return time;
		}
	} else {
		return 0.f;
	}
}
float pop_forage_time(dcon::pop_id pop, float free, float warband) {
	auto remaining = free - warband;
	auto desire = state.pop_get_forage_ratio(pop);
	if (remaining < desire) {
		return remaining;
	} else {
		return desire;
	}
}
float pop_work_time(dcon::pop_id pop, float free, float warband, float forage) {
	auto remaining = free - warband - forage;
	if (remaining < 0.f) {
		return 0.f;
	} else {
		return remaining;
	}
}

float job_efficiency(dcon::race_id race, bool female, uint8_t jobtype) {
	if (female) {
		return state.race_get_female_efficiency(race, jobtype) ;
	}
	return state.race_get_male_efficiency(race, jobtype);
}
float job_efficiency(dcon::pop_id pop, uint8_t jobtype) {
	return job_efficiency(
		state.pop_get_race(pop),
		state.pop_get_female(pop),
		jobtype
	) * age_multiplier(pop);
}

bool pop_same_location(dcon::pop_id a, dcon::pop_id b) {
	auto a_location = state.pop_location_get_location(state.pop_get_pop_location_as_pop(a));
	auto b_location = state.pop_location_get_location(state.pop_get_pop_location_as_pop(a));
	if (state.settlement_is_valid(a_location) && a_location == b_location) {
		return true;
	} // if not in same settlement, check if in same party
	auto a_warband = state.warband_unit_get_warband(state.pop_get_warband_unit_as_unit(a));
	auto b_warband = state.warband_unit_get_warband(state.pop_get_warband_unit_as_unit(b));
	if (state.warband_is_valid(a_warband) && a_warband == b_warband) {
		return true;
	}
	return false;
}

bool is_dependent_of(dcon::pop_id pop, dcon::pop_id parent) {
	auto age = age_years(pop);
	auto race = state.pop_get_race(pop);
	auto teen_age = state.race_get_teen_age(race);
	if (age < teen_age && parent && pop_same_location(pop,parent))
		return true;
	return false;
}
bool is_dependent(dcon::pop_id pop) {
	auto age = age_years(pop);
	auto race = state.pop_get_race(pop);
	auto teen_age = state.race_get_teen_age(race);
	auto parent = state.parent_child_relation_get_parent(state.pop_get_parent_child_relation_as_child(pop));
	if (age < teen_age && parent && pop_same_location(pop,parent))
		return true;
	return false;
}

void update_vegetation(float speed) {
	state.execute_serial_over_tile([speed](auto ids) {
		auto conifer = state.tile_get_conifer(ids);
		auto broadleaf = state.tile_get_broadleaf(ids);
		auto shrub = state.tile_get_shrub(ids);
		auto grass = state.tile_get_grass(ids);

		auto ideal_conifer = state.tile_get_ideal_conifer(ids);
		auto ideal_broadleaf = state.tile_get_ideal_broadleaf(ids);
		auto ideal_shrub = state.tile_get_ideal_shrub(ids);
		auto ideal_grass = state.tile_get_ideal_grass(ids);

		state.tile_set_conifer(ids, conifer * (1.f - speed) + ideal_conifer * speed);
		state.tile_set_broadleaf(ids, broadleaf * (1.f - speed) + ideal_broadleaf * speed);
		state.tile_set_shrub(ids, shrub * (1.f - speed) + ideal_shrub * speed);
		state.tile_set_grass(ids, grass * (1.f - speed) + ideal_grass * speed);
	});
}

template<typename T>
ve::fp_vector get_permeability(T tile_id) {
	ve::fp_vector tile_perm = 2.5f;
	auto sand = state.tile_get_sand(tile_id);
	auto silt = state.tile_get_silt(tile_id);
	auto clay = state.tile_get_clay(tile_id);

	tile_perm = ve::select(sand > 0.15f, tile_perm - 2.f * (sand - 0.15f) / (1.0f - 0.15f), tile_perm);
	tile_perm = ve::select(silt > 0.85f, tile_perm - 2.f * (sand - 0.15f) / (1.0f - 0.15f), tile_perm);
	tile_perm = ve::select(clay > 0.2f, tile_perm - 1.25f * (clay - 0.2f) / (1.0f - 0.2f), tile_perm);

	return tile_perm / 2.5f;
}


void apply_resource(int32_t resource_index) {
	dcon::resource_fat_id res = dcon::fatten(state, dcon::resource_id{(dcon::resource_id::value_base_t)resource_index});

	auto rng_engine = std::default_random_engine();
	auto distribution = std::uniform_real_distribution<float> {0, 1};

	auto generator = std::bind(distribution, rng_engine);

	state.execute_parallel_over_tile([&](auto tiles) {
		auto tile_is_land = state.tile_get_is_land(tiles);

		ve::mask_vector land_check {false};

		if (res.get_land()) {
			land_check = land_check || tile_is_land;
		}
		if (res.get_water()) {
			land_check = land_check || (!tile_is_land);
		}

		auto coast_check = state.tile_get_is_coast(tiles);

		auto conifers = state.tile_get_conifer(tiles);
		auto broadleaf = state.tile_get_broadleaf(tiles);
		auto trees = conifers + broadleaf;


		ve::mask_vector base_check =
		(
			(
				(
						land_check
					&&
						(
								coast_check
							||
								!res.get_coastal()
						)
				)
			&&
				(
					state.tile_get_elevation(tiles) <= res.get_maximum_elevation()
					&&
					state.tile_get_elevation(tiles) >= res.get_minimum_elevation()
				)
			)
		&&
			(
				(
					trees <= res.get_maximum_trees()
					&&
					trees >= res.get_minimum_trees()
				)
				&&
				(
					(state.tile_get_ice_age_ice(tiles) > 0)
					||
					!res.get_ice_age()
				)
			)
		);

		ve::mask_vector bedrock_check {false};
		if (!res.get_required_bedrock(0)) {
			bedrock_check = ve::mask_vector{true};
		}

		for (int i = 0; i <  state.resource_get_required_bedrock_size(); i++) {
			auto requirement = res.get_required_bedrock(i);
			if (!requirement) {
				break;
			}
			bedrock_check = bedrock_check || (state.tile_get_bedrock(tiles) == requirement);
		}

		ve::mask_vector biome_check {false};
		if (!res.get_required_biome(0)) {
			biome_check = ve::mask_vector{true};
		}

		for (int i = 0; i <  state.resource_get_required_biome_size(); i++) {
			auto requirement = res.get_required_biome(i);
			if (!requirement) {
				break;
			}
			biome_check = biome_check || (state.tile_get_biome(tiles) == requirement);
		}

		auto result = base_check && bedrock_check && biome_check;

		auto dice_roll = ve::apply([&](auto tile) {
			return generator() < 1.f / res.get_base_frequency();
		}, tiles);

		ve::value_to_vector_type<dcon::resource_id> current = state.tile_get_resource(tiles);
		ve::value_to_vector_type<dcon::resource_id> candidate = res.id;

		state.tile_set_resource(tiles, ve::select(result && dice_roll, candidate, current));
	});
}

void apply_biome(int32_t biome_index) {
	dcon::biome_fat_id biome = dcon::fatten(state, dcon::biome_id{(uint8_t)biome_index});
	assert(state.biome_is_valid(biome));

	state.execute_parallel_over_tile([&biome](auto ids) {

		auto trees = state.tile_get_broadleaf(ids) + state.tile_get_conifer(ids);
		auto dead_land = 1 - trees - state.tile_get_shrub(ids) - state.tile_get_grass(ids);
		auto conifer_fraction = ve::select(trees == 0, 0.5f, state.tile_get_conifer(ids) / trees);

		auto jan_temp = state.tile_get_january_temperature(ids);
		auto jan_rain = state.tile_get_january_rain(ids);
		auto jul_temp = state.tile_get_july_temperature(ids);
		auto jul_rain = state.tile_get_july_temperature(ids);

		auto rain = (jan_rain + jul_rain) * 0.5f;
		auto temperature = (jan_temp + jul_temp) / 2;
		auto summer_temperature = ve::max(jan_temp, jul_temp);
		auto winter_temperature = ve::min(jan_temp, jul_temp);

		auto permeability = get_permeability(ids);

		auto available_water = rain * 2 * permeability;

		auto soil_depth = state.tile_get_sand(ids) + state.tile_get_silt(ids) + state.tile_get_clay(ids);

		ve::mask_vector biome_mask =
			(
				(
					(
						(
							(
								state.tile_get_slope(ids) > biome.get_minimum_slope()
							&&
								state.tile_get_slope(ids) < biome.get_maximum_slope()
							)
						&&
							(
								state.tile_get_is_land(ids) != biome.get_aquatic()
							&&
								state.tile_get_has_marsh(ids) == biome.get_marsh()
							)
						)
					&&
						(
							(
								state.tile_get_elevation(ids) > biome.get_minimum_elevation()
							&&
								state.tile_get_elevation(ids) < biome.get_maximum_elevation()
							)
						&&
							(
								state.tile_get_sand(ids) > biome.get_minimum_sand()
							&&
								state.tile_get_sand(ids) < biome.get_maximum_sand()
							)
						)
					)
				&&
					(
						(
							(
								state.tile_get_clay(ids) > biome.get_minimum_clay()
							&&
								state.tile_get_clay(ids) < biome.get_maximum_clay()
							)
						&&
							(
								state.tile_get_silt(ids) > biome.get_minimum_silt()
							&&
								state.tile_get_silt(ids) < biome.get_maximum_silt()
							)
						)
					&&
						(
							(
								state.tile_get_shrub(ids) > biome.get_minimum_shrubs()
							&&
								state.tile_get_shrub(ids) < biome.get_maximum_shrubs()
							)
						&&
							(
								state.tile_get_grass(ids) > biome.get_minimum_grass()
							&&
								state.tile_get_grass(ids) < biome.get_maximum_grass()
							)
						)
					)
				)
			&&
				(
					(
						(
							(
								trees > biome.get_minimum_trees()
							&&
								trees < biome.get_maximum_trees()
							)
						&&
							(
								dead_land > biome.get_minimum_dead_land()
							&&
								dead_land < biome.get_maximum_dead_land()
							)
						)
					&&
						(
							(
								conifer_fraction > biome.get_minimum_conifer_fraction()
							&&
								conifer_fraction < biome.get_maximum_conifer_fraction()
							)
						&&
							(
								rain > biome.get_minimum_rain()
							&&
								rain < biome.get_maximum_rain()
							)
						)
					)
				&&
					(
						(
							(
								temperature > biome.get_minimum_temperature()
							&&
								temperature < biome.get_maximum_temperature()
							)
						&&
							(
								summer_temperature > biome.get_minimum_summer_temperature()
							&&
								summer_temperature < biome.get_maximum_summer_temperature()
							)
						)
					&&
						(
							(
								winter_temperature > biome.get_minimum_winter_temperature()
							&&
								winter_temperature < biome.get_maximum_winter_temperature()
							)
						// &&
						// 	(
						// 		state.tile_get_shrub(ids) > biome.get_minimum_grass()
						// 	&&
						// 		state.tile_get_shrub(ids) < biome.get_maximum_grass()
						// 	)
						)
					)
				)
			)
		&&
			(
				(
					(
						(
							available_water > biome.get_minimum_available_water()
						)
						&&
						(
							available_water < biome.get_maximum_available_water()
						)
					)
					&&
					(
						(
							soil_depth > biome.get_minimum_soil_depth()
						)
						&&
						(
							soil_depth < biome.get_maximum_soil_depth()
						)
					)
				)
				&&
				(
					(
						state.tile_get_soil_minerals(ids) > biome.get_minimum_soil_richness()
						&&
						state.tile_get_soil_minerals(ids) < biome.get_maximum_soil_richness()
					)
					&&
					biome.get_icy() == (state.tile_get_ice(ids) > 0.001)
				)
			);

		ve::value_to_vector_type<dcon::biome_id> current = state.tile_get_biome(ids);
		ve::value_to_vector_type<dcon::biome_id> candidate = biome.id;

		state.tile_set_biome(ids, ve::select(biome_mask, candidate, current));
	});
}

float price_score(float price) {
	return std::min(1.f, 1000.f / price);
}
ve::fp_vector price_score(ve::fp_vector price) {
	return ve::min(1.f, 1000.f / price);
}

void pops_consume() {
	static auto uses_buffer = state.trade_good_category_make_vectorizable_float_buffer();

	state.for_each_pop([&](auto pop){
		if (is_dependent(pop)) return;

		// std::cout << "pop: " << pop.index();

		for (uint32_t i = 0; i < state.pop_get_need_satisfaction_size(); i++) {
			base_types::need_satisfaction& need = state.pop_get_need_satisfaction(pop, i);
			// std::cout << "need: " << i << " " << need.use_case;

			if (need.use_case == 0)	break;

			auto demanded = need.demanded;

			auto use = dcon::use_case_id{dcon::use_case_id::value_base_t(need.use_case - 1)};

			state.pop_for_each_parent_child_relation_as_parent(pop, [&](auto child_rel) {
				auto child = state.parent_child_relation_get_child(child_rel);
				if (is_dependent_of(pop,child)) {
					base_types::need_satisfaction& need_child = state.pop_get_need_satisfaction(child, i);
					demanded += need_child.demanded;
					// transfer half of relevent trade goods for collective satisfaction
					state.use_case_for_each_use_weight_as_use_case(use, [&](auto weight_id){
						auto trade_good = state.use_weight_get_trade_good(weight_id);
						auto amount = state.pop_get_inventory(child,trade_good);
						state.pop_set_inventory(child,trade_good,amount*0.5);
						state.pop_set_inventory(pop,trade_good,state.pop_get_inventory(pop,trade_good)+amount*0.5);
					});
				}
			});


			auto actual_consumption_rate = state.use_case_get_good_consumption(use);
			auto satisfied = 0.f;

			state.use_case_for_each_use_weight_as_use_case(use, [&](auto weight_id){
				auto weight = state.use_weight_get_weight(weight_id);
				auto trade_good = state.use_weight_get_trade_good(weight_id);

				auto inventory = state.pop_get_inventory(pop, trade_good);
				auto can_consume = inventory * weight;

				if (satisfied >= demanded) {
					return;
				} else if (satisfied + can_consume > demanded) {
					auto consumed = (demanded - satisfied) / weight * actual_consumption_rate;
					state.pop_set_inventory(pop, trade_good, std::max(0.f, inventory - consumed));
					satisfied = demanded;
					return;
				} else {
					satisfied += can_consume;
					auto consumed = inventory * actual_consumption_rate;
					state.pop_set_inventory(pop, trade_good, std::max(0.f, inventory - consumed));
				}
			});

			auto satisfaction = satisfied / demanded;

			need.consumed = need.demanded * satisfaction;
			state.pop_for_each_parent_child_relation_as_parent(pop, [&](auto child_rel) {
				auto child = state.parent_child_relation_get_child(child_rel);
				auto child_age = age_years(child);
				auto teen_age = state.race_get_teen_age(state.pop_get_race(child));
				if (child_age < teen_age) {
					base_types::need_satisfaction& need_child = state.pop_get_need_satisfaction(child, i);
					need_child.consumed = need.demanded * satisfaction;
				}
			});
		}
	});
}

void pops_update_stats() {
	state.for_each_pop([&](auto pop) {
		auto total_basic_consumed = 0.f;
		auto total_basic_demanded = 0.f;

		auto total_life_demanded = 0.f;
		auto total_life_consumed = 0.f;

		for (uint32_t i = 0; i < state.pop_get_need_satisfaction_size(); i++) {
			base_types::need_satisfaction& need = state.pop_get_need_satisfaction(pop, i);
			if (need.use_case == 0) break;

			auto need_id = dcon::need_id{dcon::need_id::value_base_t(int(need.need) - 1)};

			if (state.need_get_life_need(need_id)){
				total_life_consumed += need.consumed;
				total_life_demanded += need.demanded;
			} else {
				total_basic_consumed += need.consumed;
				total_basic_demanded += need.demanded;
			}
		}

		auto life_satisfaction = total_life_consumed / total_life_demanded;
		auto basic_satisfaction = (total_basic_consumed + total_life_consumed) / (total_basic_demanded + total_life_demanded);
		state.pop_set_life_needs_satisfaction(pop, life_satisfaction);
		state.pop_set_basic_needs_satisfaction(pop, basic_satisfaction);

		// shift foraging based on life satisfaction
		auto forage_ratio = state.pop_get_forage_ratio(pop);
		if (life_satisfaction < 0.5f) {
			forage_ratio *= 1.05f;
		} else if (life_satisfaction >= 1.f) {
			forage_ratio *= 0.95f;
		}
		if (forage_ratio < 0.05f) forage_ratio = 0.05f;
		else if (forage_ratio > 0.95f) forage_ratio = 0.95f;
		state.pop_set_forage_ratio(pop, forage_ratio);
	});
}

auto WORKERS_SHARE = 0.01f;
// estates can interact only with local pops
// can do in parallel over settlements
void estates_pay() {
	state.for_each_estate([&](auto estate) {
		auto savings = state.estate_get_savings(estate);

		auto wage_budget = savings * WORKERS_SHARE;
		state.estate_get_balance_last_tick(estate) -= wage_budget;

		float total_work_time = 0.f;
		state.estate_for_each_building_estate(estate, [&](auto building_location) {
			auto building = state.building_estate_get_building(building_location);
			auto worker = state.building_get_worker_from_employment(building);
			if (worker) {
				total_work_time += state.pop_get_work_ratio(worker);
			}
		});

		if (total_work_time < 0.01f) {
			return;
		}

		state.estate_for_each_building_estate(estate, [&](auto building_location) {
			auto building = state.building_estate_get_building(building_location);
			auto worker = state.building_get_worker_from_employment(building);
			if (worker) {
				auto work_ratio = state.pop_get_work_ratio(worker);
				auto share = wage_budget * work_ratio / total_work_time;
				state.pop_get_pending_economy_income(worker) += share;
				state.building_set_worker_income_from_employment(building, share);
			}
		});
	});
}

// TODO: rewrite more stuff to parallel loops, as there are a lot of opportunities for parallelisation
void update_economy() {
	uint32_t trade_goods_count = state.trade_good_size();

	state.execute_serial_over_estate([&](auto estates) {
		state.estate_set_balance_last_tick(estates, 0.f);
	});
	concurrency::parallel_for(uint32_t(0), state.trade_good_size(), [&](auto trade_good_raw_id) {
		dcon::trade_good_id trade_good { dcon::trade_good_id::value_base_t(trade_good_raw_id) };
		if (!state.trade_good_is_valid(trade_good)) return;
		state.execute_serial_over_estate([&](auto estates) {
			state.estate_set_inventory_demanded_last_tick(estates, trade_good, 0.f);
			state.estate_set_inventory_sold_last_tick(estates, trade_good, 0.f);
			state.estate_set_inventory_bought_last_tick(estates, trade_good, 0.f);
		});
	});

	// update pops self value
	state.execute_serial_over_pop([&](auto pops) {
		state.pop_set_expected_wage(pops, ve::max(state.pop_get_savings(pops) * 0.01f, state.pop_get_expected_wage(pops)));
	});

	auto eps = 0.001f;
	const float pop_donation = 0.05f;

	// decay inventories of producers
	concurrency::parallel_for(uint32_t(0), trade_goods_count, [&](auto good_id){
		dcon::trade_good_id trade_good{ dcon::trade_good_id::value_base_t(good_id) };
		float inventory_decay = state.trade_good_get_decay(trade_good);
		state.execute_serial_over_pop([&](auto ids){
			auto inventory = state.pop_get_inventory(ids, trade_good);
			state.pop_set_inventory(ids, trade_good, inventory * inventory_decay);
		});
		state.execute_serial_over_estate([&](auto ids){
			auto inventory = state.estate_get_inventory(ids, trade_good);
			state.estate_set_inventory(ids, trade_good, inventory * inventory_decay);
		});
	});

	// decay inventories in settlements and realms:
	concurrency::parallel_for(uint32_t(0), trade_goods_count, [&](auto good_id){
		dcon::trade_good_id trade_good{ dcon::trade_good_id::value_base_t(good_id) };
		float inventory_decay = state.trade_good_get_decay(trade_good);
		state.execute_serial_over_settlement([&](auto ids){
			auto stockpiles = state.settlement_get_local_storage(ids, trade_good);
			state.settlement_set_local_storage(ids, trade_good, stockpiles * inventory_decay);
		});
		state.execute_serial_over_realm([&](auto ids){
			auto stockpiles = state.realm_get_resources(ids, trade_good);
			state.realm_set_resources(ids, trade_good, stockpiles * inventory_decay);
		});
	});

	pops_consume();
	estates_pay();
	pops_update_stats();
}
