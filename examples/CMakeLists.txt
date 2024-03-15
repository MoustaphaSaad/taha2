add_executable(playground playground.cpp)
target_link_libraries(playground core)

add_executable(ws-server ws-server.cpp)
target_link_libraries(ws-server core)

add_executable(ws-client2 ws-client2.cpp)
target_link_libraries(ws-client2 core)

add_executable(thread-per-client-echo-server thread-per-client-echo-server.cpp)
target_link_libraries(thread-per-client-echo-server core)

add_executable(ping-pong ping-pong.cpp)
target_link_libraries(ping-pong core)

add_executable(single-threaded-echo-server single-threaded-echo-server.cpp)
target_link_libraries(single-threaded-echo-server core)

add_executable(multi-threaded-echo-server multi-threaded-echo-server.cpp)
target_link_libraries(multi-threaded-echo-server core)

add_executable(ws-server2 ws-server2.cpp)
target_link_libraries(ws-server2 core)

add_executable(ws-client3 ws-client3.cpp)
target_link_libraries(ws-client3 core)