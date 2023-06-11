add_executable(unittests
	main.cpp
	test_allocator.cpp
	test_unique.cpp
)

CPMGetPackage(doctest)
target_link_libraries(unittests PRIVATE doctest core)