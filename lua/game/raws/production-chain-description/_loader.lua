return function ()
	print("loading chains")

	print("iron")
	require "game.raws.production-chain-description.iron"()

	print("gold")
	require "game.raws.production-chain-description.gold"()

	print("silver")
	require "game.raws.production-chain-description.silver"()

	print("arsenic bronze")
	require "game.raws.production-chain-description.alloy-arsenic-bronze"()

	print("tin bronze")
	require "game.raws.production-chain-description.alloy-tin-bronze"()

	print("brass")
	require "game.raws.production-chain-description.alloy-brass"()
end