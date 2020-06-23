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
    std::cout<<log<<std::endl;
    return 0;
}