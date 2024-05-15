add_executable(budget-byte-cli main.cpp)
CPMGetPackage(date)
target_link_libraries(budget-byte-cli core budget-byte date)