
add_executable(my_test my_test.cpp common/test.cpp)
target_link_libraries(my_test ABY::aby ENCRYPTO_utils::encrypto_utils)
