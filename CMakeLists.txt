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

add_executable(pmon src/monitor.c src/input.c src/syscall_handler.c src/pr_tree.c)
target_include_directories(pmon PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(test_nested_forks test/programs/test_nested_forks.c)

enable_testing()

add_executable(test_input test/test_input.c src/input.c)
add_dependencies(test_input criterion)
target_link_libraries(test_input PRIVATE criterion)
target_include_directories(test_input PRIVATE ${criterion_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/include)
add_test(NAME input_tests COMMAND test_input)

add_executable(test_syscall_handler test/test_syscall_handler.c src/syscall_handler.c src/pr_tree.c)
add_dependencies(test_syscall_handler criterion)
target_link_libraries(test_syscall_handler PRIVATE criterion)
target_include_directories(test_syscall_handler PRIVATE ${criterion_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/include)
add_test(NAME syscall_handler_tests COMMAND test_syscall_handler)

