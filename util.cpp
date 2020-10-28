#include "util.h"
#include <sstream>
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

    std::ostringstream oss;
    oss << std::this_thread::get_id();
    std::cout << buf << oss.str() << " ====> " << log  << std::endl;
    return 0;
}
std::string MD5(const std::string& src )
{
    MD5_CTX ctx;
    std::string md5_string;
    std::string dst;
    unsigned char md[16] = { 0 };
    char tmp[33] = { 0 };

    MD5_Init( &ctx );
    MD5_Update( &ctx, src.c_str(), src.size() );
    MD5_Final( md, &ctx );

    for( int i = 0; i < 16; ++i )
    {   
        memset( tmp, 0x00, sizeof( tmp ) );
        sprintf( tmp, "%02X", md[i] );
        md5_string += tmp;
    }   
    std::transform(md5_string.begin(), md5_string.end(), dst.begin(), ::tolower);
    return dst;
}
void ms_sleep(unsigned ms)
{
    struct timeval tv;
    tv.tv_sec=0;
    tv.tv_usec=ms * 1000;
    int err;
    do{
       err=select(0,NULL,NULL,NULL,&tv);
    }while(err<0 && errno==EINTR);
}