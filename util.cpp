#include "util.h"
uint64_t gettime()
{
    timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec*1000 + tv.tv_usec/1000;//ms
}
int log(lua_State *L)
{
    const char *log = lua_tostring(L, -1);
    time_t now_time = time(NULL);
    char buf[128] = {0};
    tm *local = localtime(&now_time);
    strftime(buf, 128, "%Y-%m-%d %H:%M:%S", local);
    std::cout << buf << " ====> " << log  << std::endl;
    return 0;
}