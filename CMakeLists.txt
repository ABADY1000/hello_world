# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
include_directories(src/inc)
project(hello_world)

file(GLOB Dir1_Sources "src/lib/*.c")
add_executable(hello_world ${Dir1_Sources})

target_sources(app PRIVATE src/main.c)
