add_executable(playground playground.cpp)
target_link_libraries(playground core)

add_executable(thread-per-client-echo-server thread-per-client-echo-server.cpp)
target_link_libraries(thread-per-client-echo-server core)

add_executable(ws-server ws-server.cpp)
target_link_libraries(ws-server core)

add_executable(ws-client ws-client.cpp)
target_link_libraries(ws-client core)

add_executable(select select.cpp)
target_link_libraries(select core)
