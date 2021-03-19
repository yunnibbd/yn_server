PROJECT := $(shell pwd)
MAIN 	:= $(PROJECT)/yn_server.cpp
SRC  	:= $(wildcard $(PROJECT)/*.cpp $(PROJECT)/misc/*.cpp $(PROJECT)/net/*.cpp $(PROJECT)/net/*.c $(PROJECT)/component/*.cpp $(PROJECT)/utils/*.cpp)
override SRC := $(filter-out $(MAIN), $(SRC))
OBJECT  := $(patsubst %.cpp %.c, %.o, $(SRC))
BIN 	:= $(PROJECT)/bin
TARGET  := yn_server
CXX     := g++
LIBS    := -lpthread
INCLUDE	:= -I ./usr/local/lib
CFLAGS  := -std=c++11 -g -pg -Wall -O3 -D_PTHREADS
CXXFLAGS:= $(CFLAGS)

all : $(BIN)/$(TARGET)

$(BIN)/$(TARGET) : $(OBJECT) $(PROJECT)/yn_server.o
	[ -e $(BIN) ] || mkdir $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

clean :
	find . -name '*.o' | xargs rm -f
	find . -name $(TARGET) | xargs rm -f
