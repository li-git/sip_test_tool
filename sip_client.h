#ifndef __SIP_CLIENT_H__
#define __SIP_CLIENT_H__
#include "connect_handle.h"

int sendmsg(lua_State *L);
class sip_client;
class net_poll
{
public:
    net_poll();
    bool loop();
    bool add_client(sip_client *c);
    bool del_client(sip_client *c);
public:
    int epfd;
    epoll_event *revents;
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
    bool resume_lua_state(lua_State *L);
    int on_read();
    void send_msg(std::string &msg);
    bool run_script();
public:
    connect_base *connect;
    string buf;
    lua_State *L;
    net_poll *m_netpoll;
};
#endif