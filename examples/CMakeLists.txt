add_executable(playground playground.cpp)
target_link_libraries(playground core)

add_executable(ws-server ws-server.cpp)
target_link_libraries(ws-server core)

add_executable(ws-client ws-client.cpp)
target_link_libraries(ws-client core)

add_executable(ws-client2 ws-client2.cpp)
target_link_libraries(ws-client2 core)

add_executable(echo-server echo-server.cpp)
target_link_libraries(echo-server core)

add_executable(threaded-echo-server threaded-echo-server.cpp)
target_link_libraries(threaded-echo-server core)