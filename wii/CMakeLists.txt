cmake_minimum_required(VERSION 3.13)
project(LocoRocoWii C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT COMMAND ogc_create_dol)
    message(FATAL_ERROR "ogc_create_dol not found. Use Wii.cmake toolchain.")
endif()

add_definitions(-D__wii__ -DGEKKO)

file(GLOB SOURCES "source/*.cpp" "source/*.c")

add_executable(locoroco-wii ${SOURCES})

target_include_directories(locoroco-wii PRIVATE
    source
    $ENV{DEVKITPRO}/libogc/include
    $ENV{DEVKITPRO}/portlibs/ppc/include
    $ENV{DEVKITPRO}/portlibs/wii/include
)

target_link_directories(locoroco-wii PRIVATE
    $ENV{DEVKITPRO}/portlibs/ppc/lib
    $ENV{DEVKITPRO}/portlibs/wii/lib
    $ENV{DEVKITPRO}/libogc/lib/wii
)

target_link_libraries(locoroco-wii PRIVATE
    box2d
    wiiuse
    asnd
    bte
    fat
    ogc
    m
    stdc++
)

target_compile_options(locoroco-wii PRIVATE
    -O2
    -Wall
    -fno-rtti
    -fno-exceptions
)

ogc_create_dol(locoroco-wii)

set_source_files_properties(source/RuntimeAssets.cpp PROPERTIES COMPILE_OPTIONS "-O0")
