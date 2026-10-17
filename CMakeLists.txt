cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(KodcuKeypad VERSION 1.0.0)

# Clang 21 (NDK 29) fmt kutuphanesinin kendi kodunu derleyemiyor (consteval hatasi).
# consteval'i kapatarak bu hatayi atliyoruz. Mutlaka add_subdirectory'den ONCE olmali.
add_compile_definitions(FMT_USE_CONSTEVAL=0 FMT_CONSTEVAL=)

file(GLOB SOURCES src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "GEODE_SDK ortam degiskeni tanimli degil")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
setup_geode_mod(${PROJECT_NAME})
