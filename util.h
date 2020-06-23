extern "C"
{
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <unistd.h>
#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
#include <list>
#include <map>
#include <set>

uint64_t gettime();
int log(lua_State *L);