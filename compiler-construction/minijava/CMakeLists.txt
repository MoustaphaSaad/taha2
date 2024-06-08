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
  include/minijava/Parser.h
  include/minijava/ASTPrinter.h
  src/minijava/Parser.cpp
  src/minijava/Scanner.cpp
  src/minijava/Unit.cpp
)
target_link_libraries(minijava PRIVATE core)
target_include_directories(minijava PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")

enable_testing()

add_test(NAME scan-sample COMMAND
  minijava scan --test ${TESTS_DIR}/scan/sample.minijava
)

add_test(NAME scan-sample-error COMMAND
  minijava scan --test ${TESTS_DIR}/scan/sample-error.minijava
)

add_test(NAME parse-expr-add COMMAND
  minijava parse-expr --test ${TESTS_DIR}/parse-expr/add.minijava
)

add_test(NAME parse-expr-add-mul COMMAND
  minijava parse-expr --test ${TESTS_DIR}/parse-expr/add-mul.minijava
)

add_test(NAME parse-expr-array-length COMMAND
  minijava parse-expr --test ${TESTS_DIR}/parse-expr/array-length.minijava
)

add_test(NAME parse-expr-array-lookup COMMAND
  minijava parse-expr --test ${TESTS_DIR}/parse-expr/array-lookup.minijava
)

add_test(NAME parse-expr-call COMMAND
  minijava parse-expr --test ${TESTS_DIR}/parse-expr/call.minijava
)

add_test(NAME parse-expr-false COMMAND
  minijava parse-expr --test ${TESTS_DIR}/parse-expr/false.minijava
)

add_test(NAME parse-expr-id COMMAND
  minijava parse-expr --test ${TESTS_DIR}/parse-expr/id.minijava
)

add_test(NAME parse-expr-int COMMAND
  minijava parse-expr --test ${TESTS_DIR}/parse-expr/int.minijava
)

add_test(NAME parse-expr-new-array COMMAND
  minijava parse-expr --test ${TESTS_DIR}/parse-expr/new-array.minijava
)

add_test(NAME parse-expr-new-object COMMAND
  minijava parse-expr --test ${TESTS_DIR}/parse-expr/new-object.minijava
)

add_test(NAME parse-expr-not COMMAND
  minijava parse-expr --test ${TESTS_DIR}/parse-expr/not.minijava
)

add_test(NAME parse-expr-paren COMMAND
  minijava parse-expr --test ${TESTS_DIR}/parse-expr/paren.minijava
)

add_test(NAME parse-expr-sub COMMAND
  minijava parse-expr --test ${TESTS_DIR}/parse-expr/sub.minijava
)

add_test(NAME parse-expr-this COMMAND
  minijava parse-expr --test ${TESTS_DIR}/parse-expr/this.minijava
)

add_test(NAME parse-expr-true COMMAND
  minijava parse-expr --test ${TESTS_DIR}/parse-expr/true.minijava
)