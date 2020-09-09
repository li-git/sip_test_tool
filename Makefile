SOURCE  := $(wildcard *.c) $(wildcard *.cpp)
OBJS    := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))

TARGET  := tool

CC      := gcc
CXX     := g++ -std=c++11
LIBS    := -pthread -ldl -lsrtp -lcurl -luuid -lssl -lcrypto -llua
LIBS    += -lpcap -lmysqlclient -lmysqlservices
LDFLAGS := -L. -L./lib -L./lib/libs -L/usr/lib64/mysql
INCLUDE := -I./include
DEFINES := #-D__LINUX__
CFLAGS  := -g $(INCLUDE) $(DEFINES)
CXXFLAGS:= $(CFLAGS)

all : $(TARGET)

objs : $(OBJS)

clean :
	rm -rf *.o
	rm -rf $(TARGET)
	rm -rf *.log

$(TARGET) : $(OBJS)
	$(CXX) -o $@ $(OBJS) $(DEFINES) $(LDFLAGS) $(LIBS)
