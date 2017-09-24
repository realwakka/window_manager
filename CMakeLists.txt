cmake_minimum_required (VERSION 3.1)

project(event_loop)

include_directories(${CMAKE_SOURCE_DIR})

set (CMAKE_CXX_STANDARD 14)

add_library(el_server
  server.cc
  session.cc
  )
target_link_libraries(el_server el_drm boost_system pthread)

add_executable(server_test main.cc)
target_link_libraries(server_test el_server)

add_subdirectory(client)
add_subdirectory(drm)
