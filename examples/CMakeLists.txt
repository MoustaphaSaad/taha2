add_executable(playground playground.cpp)
target_link_libraries(playground core)

add_executable(ws-server ws-server.cpp)
target_link_libraries(ws-server core)

add_executable(ws-client ws-client.cpp)
target_link_libraries(ws-client core)

add_executable(echo-server echo-server.cpp)
target_link_libraries(echo-server core)