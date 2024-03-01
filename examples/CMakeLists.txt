add_executable(playground playground.cpp)
target_link_libraries(playground core)

add_executable(ws-server ws-server.cpp)
target_link_libraries(ws-server core)

add_executable(ws-client2 ws-client2.cpp)
target_link_libraries(ws-client2 core)

add_executable(echo-server echo-server.cpp)
target_link_libraries(echo-server core)

add_executable(threaded-echo-server threaded-echo-server.cpp)
target_link_libraries(threaded-echo-server core)

add_executable(ping-pong ping-pong.cpp)
target_link_libraries(ping-pong core)

add_executable(single-threaded-echo-server single-threaded-echo-server.cpp)
target_link_libraries(single-threaded-echo-server core)