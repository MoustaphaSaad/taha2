add_executable(unittests
	main.cpp
	test_allocator.cpp
	test_unique.cpp
	test_shared.cpp
	test_array.cpp
	test_hash.cpp
	test_rune.cpp
	test_string.cpp
	test_osstring.cpp
	test_file.cpp
)

CPMGetPackage(doctest)
target_link_libraries(unittests PRIVATE doctest core)