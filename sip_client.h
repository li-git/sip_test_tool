#ifndef __SIP_CLIENT_H__
#define __SIP_CLIENT_H__
#include "connect_handle.h"
#include "timer.h"

int sendmsg(lua_State *L);
class sip_client;
class net_poll
{
public:
    net_poll();
    bool loop();
    bool epoll_add(sip_client *c);
    bool epoll_del(sip_client *c, int fd);
public:
    int epfd;
    //std::unordered_map<int, sip_client*> cli_map;
};

enum Protocol
{
    T_TCP = 0,
    T_TLS
};
class sip_client 
{
public:
    sip_client(Protocol type, std::string &ipaddr, int port, std::string &path, net_poll *np);
    ~sip_client();
    bool on_sip_msg(const char *sip_msg);
    int on_read();
    void send_msg(std::string &msg);
    bool run_script(int argc = 0);
    bool operator<(const sip_client &c);
    int getfd();
    bool inject_values(int index);
public:
    connect_base *m_connect;
    string buf;
    lua_State *L;
    net_poll *m_netpoll;
    int64_t end_time;
    void *user_data;
};
#endif