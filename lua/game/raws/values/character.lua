local pop_utils = require "game.entities.pop".POP

local character_values = {}

---commenting
---@param character Character
---@return boolean
function character_values.is_traveller(character)
	for i = 1, MAX_TRAIT_INDEX do
		local trait = DATA.pop_get_traits(character, i)

		if trait == INVALID_ID then
			break
		end

		local traveller = DATA.trait_get_traveller(trait)

		if traveller > 0 then
			return true
		end
	end
	return false
end

---commenting
---@param character Character
---@return number
function character_values.admin_score(character)
	local total = 0
	for i = 1, MAX_TRAIT_INDEX do
		local trait = DATA.pop_get_traits(character, i)
		if trait == INVALID_ID then
			break
		end
		total = total + DATA.trait_get_admin(trait)
	end
	return total
end

---commenting
---@param character Character
---@return number
function character_values.ambition_score(character)
	local total = 0
	for i = 1, MAX_TRAIT_INDEX do
		local trait = DATA.pop_get_traits(character, i)
		if trait == INVALID_ID then
			break
		end
		total = total + DATA.trait_get_ambition(trait)
	end
	return total
end

---commenting
---@param character Character
---@return number
function character_values.aggression_score(character)
	local total = 0
	for i = 1, MAX_TRAIT_INDEX do
		local trait = DATA.pop_get_traits(character, i)
		if trait == INVALID_ID then
			break
		end
		total = total + DATA.trait_get_aggression(trait)
	end
	return total
end

---Modifies desired profit of characters
---@param character Character
---@return number
function character_values.profit_desire(character)
	local total = 0
	for i = 1, MAX_TRAIT_INDEX do
		local trait = DATA.pop_get_traits(character, i)
		if trait == INVALID_ID then
			break
		end
		total = total + DATA.trait_get_greed(trait)
	end
	return total
end

---Calculates travel speed of given character
---@param character Character
---@return speed
function character_values.travel_speed(character)
	return pop_utils.get_speed(character)
end

---Calculates travel speed of given race  \
-- Used for diplomatic actions when there is no moving character: only abstract "diplomat"  \
-- To be removed when we will have actual diplomats.
---@param race Race
---@return speed
function character_values.travel_speed_race(race)
	return {
		base = 1,
		can_fly = false,
		forest_fast = DATA.race_get_requires_large_forest(race),
		river_fast = DATA.race_get_requires_large_river(race)
	}
end

return character_values