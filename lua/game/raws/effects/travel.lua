local travel_effects = {}


---@param party warband_id
---@param target tile_id
function travel_effects.move_party(party, target)
	local location =  DATA.get_warband_location_from_warband(party)
	DATA.warband_location_set_location(location, target)
end

---commenting
---@param character Character
function travel_effects.exit_settlement(character)
	local warband = LEADER_OF_WARBAND(character)
	if warband == INVALID_ID then
		return
	end

	local province = PROVINCE(character)
	if province == INVALID_ID then
		return
	end

	DATA.warband_set_in_settlement(warband,false)

	DATA.for_each_warband_unit_from_warband(warband, function (item)
		local unit = DATA.warband_unit_get_unit(item)

		DATA.delete_pop_location(DATA.get_pop_location_from_pop(unit))
		if IS_CHARACTER(unit) then
			DATA.delete_character_location(DATA.get_character_location_from_character(unit))
		end

		-- automatically recruit dependents as followers when leaving settlement
		DATA.for_each_parent_child_relation_from_parent(unit,function(child_rel)
			local child = DATA.parent_child_relation_get_child(child_rel)
			if IS_DEPENDENT_OF(child,unit) and UNIT_OF(child)==INVALID_ID then
				require "game.raws.effects.demography".recruit(child,warband,UNIT_TYPE.FOLLOWER)
				DATA.delete_pop_location(DATA.get_pop_location_from_pop(child))
				if IS_CHARACTER(child) then
					DATA.delete_character_location(DATA.get_character_location_from_character(child))
				end
			end
		end);
	end)
end

---commenting
---@param character Character
function travel_effects.enter_settlement(character)
	local warband = LEADER_OF_WARBAND(character)
	if warband == INVALID_ID then
		return
	end

	local province = PROVINCE(character)
	if province ~= INVALID_ID then
		return
	end

	local tile = WARBAND_TILE(warband)
	local local_province = TILE_PROVINCE(tile)

	if PROVINCE_REALM(local_province) == INVALID_ID then
		return
	end

	DATA.warband_set_in_settlement(warband,true)

	DATA.for_each_warband_unit_from_warband(warband, function (item)
		local unit = DATA.warband_unit_get_unit(item)

		DATA.force_create_pop_location(local_province, unit)
		if (IS_CHARACTER(unit)) then
			DATA.force_create_character_location(local_province, unit)
		end
		-- automatically unrecruit non dependent followers if at home
		if DATA.warband_unit_get_type(item) == UNIT_TYPE.FOLLOWER then
			local home = HOME(unit)
			if home~=INVALID_ID and home==province and not IS_DEPENDENT(unit) then
				require "game.raws.effects.demography".unrecruit(unit)
			end
		end
	end)
end

return travel_effects