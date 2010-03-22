
all: jsondb.exe

jsondb.exe: JsonDb.cpp main.cpp
	g++ -g -Wall -o jsondb.exe JsonDb.cpp main.cpp  -I /usr/include/boost-1_33_1/ -lqdbm

clean:
	rm jsondb.exe
