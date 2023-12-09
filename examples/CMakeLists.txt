add_executable(playground playground.cpp)
target_link_libraries(playground core)

add_executable(ws-server ws-server.cpp)
target_link_libraries(ws-server core)