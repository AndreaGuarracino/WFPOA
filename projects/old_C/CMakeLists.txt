cmake_minimum_required(VERSION 3.1)
project(wfpoa)

# Not all standard-compliant optimizations
# (not using them, I can debug the code step-by-step)

#gcc -g generates debug information to be used by GDB debugger.
# -g	default debug information
#-g3	maximal debug information
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g3")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g3")

set(CMAKE_C_SOURCE_FILE_EXTENSIONS src/main.c)
add_executable(wfpoa ${CMAKE_SOURCE_DIR}/src/main.c src/graph.c src/utils.c)
