add_executable(minijava)
target_sources(minijava PRIVATE
  src/minijava/main.cpp
  include/minijava/Token.h
  include/minijava/Unit.h
  include/minijava/Error.h
  include/minijava/Scanner.h
  src/minijava/Scanner.cpp
  src/minijava/Unit.cpp
)
target_link_libraries(minijava PRIVATE core)
target_include_directories(minijava PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include")