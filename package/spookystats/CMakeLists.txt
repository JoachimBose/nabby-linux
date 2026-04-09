cmake_minimum_required(VERSION 3.23)
project(VUCTF26 C)

add_library(websitelib
        websitelib/websitelib.c
        include/websitelib.h
)

add_executable(spookystats)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

include_directories(${CMAKE_SOURCE_DIR}/include)


target_sources(spookystats 
PRIVATE
src/httpserver.c
src/application.c)

target_link_libraries(spookystats websitelib)

install(TARGETS websitelib DESTINATION lib)
install(TARGETS spookystats DESTINATION bin)
