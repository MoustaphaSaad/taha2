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
	test_buffer.cpp
	test_memorystream.cpp
	test_intrinsics.cpp
	test_msgpack.cpp
	test_uuid.cpp
	test_func.cpp
	test_thread.cpp
	test_queue.cpp
	test_threadpool.cpp
	test_log.cpp
)

CPMGetPackage(doctest)
target_link_libraries(unittests PRIVATE doctest core)