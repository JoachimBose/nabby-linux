cmake_minimum_required(VERSION 3.23)
project(VUCTF26)

add_executable(spookystats)

target_sources(spookystats 
  PRIVATE
    src/helloworld.c)

install(TARGETS spookystats DESTINATION bin)
