SOURCE  := $(wildcard *.c) $(wildcard *.cpp)
OBJS    := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))

TARGET  := siptool

CC      := gcc
CXX     := g++ -std=c++11
LIBS    := -pthread -ldl -lcmmLib -ltp -lutil  -lsipphone_sdk  -lwebsockets  -lsip4VideoGW  -lCryptoSrtp  -ldum -lresip -lrutil -lares -lsrtp -lssl_sb -lzlib -lcurl -lcrypto_sb -luuid  -ljsoncpp
LIBS    += -lpcap -lmysqlclient -lmysqlservices
LDFLAGS := -L. -L./lib -L./lib/libs -L/usr/lib64/mysql
INCLUDE := -I./include -I/home/logan/git/Common/include -I/home/logan/git/Common/platform/util/h -I/home/logan/git/Common/platform/tp/h -I/home/logan/git/Common/platform
DEFINES := -D__LINUX__  -D__LINUX_CLIENT__
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
