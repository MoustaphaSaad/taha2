add_executable(minijava)
target_sources(minijava PRIVATE
  src/minijava/main.cpp
)
target_link_libraries(minijava PRIVATE core)