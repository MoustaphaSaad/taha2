add_executable(fin-server)

target_sources(fin-server PRIVATE
	src/fin-server/main.cpp
)

target_link_libraries(fin-server core fin)