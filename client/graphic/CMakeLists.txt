add_library(wm_graphic SHARED canvas.cc)

target_link_libraries(wm_graphic boost_system boost_mpi rt)
