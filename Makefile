app:
	cmake --preset linux-debug
	# creates compile_commands.json using bear
	bear -- ninja -C out/build/linux-debug
