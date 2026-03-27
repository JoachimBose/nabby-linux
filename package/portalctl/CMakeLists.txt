cmake_minimum_required(VERSION 3.23)
project(VUCTF26)

add_executable(portalctl)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

# include_directories(${CMAKE_SOURCE_DIR}/include)

target_sources(portalctl 
PRIVATE
src/main.c)

install(TARGETS portalctl DESTINATION bin)
