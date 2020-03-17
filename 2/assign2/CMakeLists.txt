cmake_minimum_required(VERSION 3.13)
project(assign2)

set(CMAKE_CXX_STANDARD 14)

include_directories(assign2_part1)
include_directories(assign2_part2)

add_executable(assign2
        assign2_part2/calc.c
        assign2_part2/calc.h)
