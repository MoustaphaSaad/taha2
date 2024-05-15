add_executable(test-math
	main.cpp
	test_vec.cpp
)

CPMGetPackage(doctest)
target_link_libraries(test-math PUBLIC doctest math)