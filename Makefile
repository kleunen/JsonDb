
all: jsondb.exe

jsondb.exe: JsonDb.cpp main.cpp
	g++ -g -Wall -o jsondb.exe JsonDb.cpp main.cpp  -I /usr/include/boost-1_33_1/ -lqdbm -lboost_unit_test_framework-gcc-mt

	# g++ -fprofile-arcs -ftest-coverage -g -Wall -o jsondb.exe JsonDb.cpp main.cpp  -I /usr/include/boost-1_33_1/ -lqdbm -lboost_unit_test_framework-gcc-mt
clean:
	rm jsondb.exe
