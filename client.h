#ifndef __SIP_CLIENT_H__
#define __SIP_CLIENT_H__
#include "connect_handle.h"
#include "timer.h"

enum WakeType
{
    WAKE_TIMER = 0,
    WAKE_DATA,
    WAKE_START
};

enum Protocol
{
    T_TCP = 0,
    T_TLS
};
class client 
{
public:
    client(Protocol type, std::string &ipaddr, int port, std::string &path);
    ~client();
    bool on_sip_msg(const char *sip_msg);
    virtual int on_read() = 0;
    void send_msg(std::string &msg);
    bool run_script(WakeType = WAKE_DATA);
    bool operator<(const client &c);
    int getfd();
    bool inject_values(int index);
public:
    connect_base *m_connect;
    lua_State *L;
    int64_t end_time;
    void *user_data;
    int m_notifyfd;
};

class sip_client: public client
{
public:
    sip_client(Protocol type, std::string &ipaddr, int port, std::string &path)
    :client(type, ipaddr,  port, path)
    {

    }
    int on_read();
    string buf;
};
class http_client: public client
{
public:
    http_client(Protocol type, std::string &ipaddr, int port, std::string &path)
    :client(type, ipaddr,  port, path)
    {

    }
    int on_read();
    string buf;
};
#endif