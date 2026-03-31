cmake_minimum_required(VERSION 3.23)
project(NABBY26)

add_executable(helloworldtest)

target_sources(helloworldtest 
  PRIVATE
    src/helloworldtest.c)

# Find libgpiod (v1 API)
find_library(GPIOD_LIB gpiod REQUIRED)
find_path(GPIOD_INCLUDE_DIR gpiod.h REQUIRED)

# Add include path
target_include_directories(helloworldtest PRIVATE ${GPIOD_INCLUDE_DIR})

# Link the library
target_link_libraries(helloworldtest PRIVATE ${GPIOD_LIB})

install(TARGETS helloworldtest DESTINATION bin)
