cmake_minimum_required(VERSION 3.9)

project(minijava LANGUAGES CXX)

add_executable(minijava)
target_sources(minijava PRIVATE
  src/minijava/main.cpp
  include/minijava/Token.h
  include/minijava/Unit.h
  include/minijava/Error.h
  include/minijava/Scanner.h
  include/minijava/AST.h
  src/minijava/Scanner.cpp
  src/minijava/Unit.cpp
)
target_link_libraries(minijava PRIVATE core)
target_include_directories(minijava PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include")

enable_testing()
add_test(NAME scan-sample COMMAND
  minijava scan --test ./compiler-construction/minijava/tests/scan/sample.minijava
)

add_test(NAME scan-sample-error COMMAND
  minijava scan --test ./compiler-construction/minijava/tests/scan/sample-error.minijava
)