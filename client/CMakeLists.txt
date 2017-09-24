add_library(el_client client.cc)
target_link_libraries(el_client boost_system pthread)

add_executable(client_test main.cc)
target_link_libraries(client_test el_client)

