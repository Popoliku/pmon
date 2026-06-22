cmake_minimum_required(VERSION 3.14)
project(pmon C)

set(CMAKE_C_STANDARD 11)

include(FetchContent)
FetchContent_Declare(
  criterion
  GIT_REPOSITORY https://github.com/Snaipe/Criterion.git
  GIT_TAG        v2.3.3
  #SOURCE_SUBDIR  "dev/cmake/"
)
FetchContent_MakeAvailable(criterion)

add_executable(pmon src/monitor.c src/parser.c src/syscall_handler.c)

add_executable(test_nested_forks test/programs/test_nested_forks.c)

enable_testing()
add_executable(test_parser test/test_parser.c src/parser.c)
add_dependencies(test_parser criterion)
target_link_libraries(test_parser PRIVATE criterion)
target_include_directories(test_parser PRIVATE ${criterion_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/include)
add_test(NAME parser_tests COMMAND test_parser)
