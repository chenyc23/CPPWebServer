vpath %.cpp src
vpath %.h inc

CFLAGS = -I inc

SRC = main.cpp http_conn.cpp
OBJ = main.o http_conn.o
#INC = locker.h http_conn.h threadpool.h
main:$(OBJ)
	g++ -o main $(OBJ) -pthread

%.o:%.cpp
	g++ $(CFLAGS) -c $< -pthread

.PHONY:clean
clean:
	rm *.o main