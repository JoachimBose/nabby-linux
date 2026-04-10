cmake_minimum_required(VERSION 3.23)
project(VUCTF26 C)

add_executable(portalctl)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE debug)

# include_directories(${CMAKE_SOURCE_DIR}/include)

target_sources(portalctl 
PRIVATE
src/main.c
src/handlers.c)

add_compile_options(-mtpcs-frame)


install(TARGETS portalctl DESTINATION bin)
