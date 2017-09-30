cmake_minimum_required (VERSION 3.1)

project(event_loop)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


include_directories(${CMAKE_SOURCE_DIR})

set (CMAKE_CXX_STANDARD 14)

add_library(el_server SHARED
  server.cc
  session.cc
  input_manager.cc
  cursor.cc
  display_manager.cc
  )
target_link_libraries(el_server el_drm boost_system pthread boost_filesystem boost_mpi rt)

add_executable(server_test main.cc)
target_link_libraries(server_test el_server)

add_subdirectory(client)
add_subdirectory(drm)
