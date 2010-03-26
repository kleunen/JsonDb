cmake_minimum_required (VERSION 2.6)
project (HELLO) 

# search for Boost 
find_package( Boost COMPONENTS filesystem unit_test_framework)

# Link against boost libraries
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS} )

add_library(JsonDb JsonDb.cpp)
add_executable(JsonDb_unit_test  main.cpp)

target_link_libraries (
		JsonDb_unit_test
		${Boost_LIBRARIES}
		"qdbm"
		"JsonDb"
	)

