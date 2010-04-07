cmake_minimum_required (VERSION 2.6)
project (HELLO) 

# search for Boost 
find_package( Boost COMPONENTS filesystem unit_test_framework system)

# search for readline
FIND_PATH(READLINE_INCLUDE_DIR readline/readline.h)
FIND_LIBRARY(READLINE_LIBRARY NAMES readline) 

IF (READLINE_INCLUDE_DIR AND READLINE_LIBRARY)
   SET(READLINE_FOUND TRUE)
ENDIF (READLINE_INCLUDE_DIR AND READLINE_LIBRARY)

IF (READLINE_FOUND)
   IF (NOT Readline_FIND_QUIETLY)
      MESSAGE(STATUS "Found GNU readline: ${READLINE_LIBRARY}")
   ENDIF (NOT Readline_FIND_QUIETLY)
ELSE (READLINE_FOUND)
   IF (Readline_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find GNU readline")
   ENDIF (Readline_FIND_REQUIRED)
ENDIF (READLINE_FOUND)

# Link against boost libraries
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS} )

add_library(JsonDb JsonDb.cpp JsonDbValues.cpp JsonDbParser.cpp JsonDbPathParser.cpp)
add_executable(JsonDb_unit_test main.cpp)
add_executable(jsondb_console Console.cpp)

target_link_libraries (
		JsonDb
		${Boost_LIBRARIES}
		"qdbm"
	)

target_link_libraries (
		JsonDb_unit_test
		${Boost_LIBRARIES}
		"qdbm"
		"JsonDb"
	)

target_link_libraries (
		jsondb_console
		${Boost_LIBRARIES}
		${READLINE_LIBRARY}
		"qdbm"
		"JsonDb"
	)
