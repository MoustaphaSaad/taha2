add_executable(minijava)
target_sources(minijava PRIVATE
  src/minijava/main.cpp
  include/minijava/Unit.h
  src/minijava/Unit.cpp
)
target_link_libraries(minijava PRIVATE core)
target_include_directories(minijava PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include")