add_executable(costco costco.cpp)
target_link_libraries(
	costco	
	PRIVATE Boost::headers
	ABY::aby ENCRYPTO_utils::encrypto_utils)
