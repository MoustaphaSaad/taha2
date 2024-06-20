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

function (minijava_test NAME)
  add_test(NAME ${NAME} COMMAND minijava ${ARGN} WORKING_DIRECTORY ${TESTS_DIR})
endfunction ()

minijava_test(scan-sample scan --test ./scan/sample.minijava)
minijava_test(scan-sample-error scan --test ./scan/sample-error.minijava)

minijava_test(parse-expr-add parse-expr --test ./parse-expr/add.minijava)
minijava_test(parse-expr-add-mul parse-expr --test ./parse-expr/add-mul.minijava)
minijava_test(parse-expr-array-length parse-expr --test ./parse-expr/array-length.minijava)
minijava_test(parse-expr-array-lookup parse-expr --test ./parse-expr/array-lookup.minijava)
minijava_test(parse-expr-call parse-expr --test ./parse-expr/call.minijava)
minijava_test(parse-expr-false parse-expr --test ./parse-expr/false.minijava)
minijava_test(parse-expr-id parse-expr --test ./parse-expr/id.minijava)
minijava_test(parse-expr-int parse-expr --test ./parse-expr/int.minijava)
minijava_test(parse-expr-new-array parse-expr --test ./parse-expr/new-array.minijava)
minijava_test(parse-expr-new-object parse-expr --test ./parse-expr/new-object.minijava)
minijava_test(parse-expr-not parse-expr --test ./parse-expr/not.minijava)
minijava_test(parse-expr-paren parse-expr --test ./parse-expr/paren.minijava)
minijava_test(parse-expr-sub parse-expr --test ./parse-expr/sub.minijava)
minijava_test(parse-expr-this parse-expr --test ./parse-expr/this.minijava)
minijava_test(parse-expr-true parse-expr --test ./parse-expr/true.minijava)