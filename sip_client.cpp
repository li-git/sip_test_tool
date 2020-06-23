#include "sip_client.h"

int sendmsg(lua_State *L)
{
    std::string msg_ = lua_tostring(L, -1);
    lua_getfield(L, LUA_REGISTRYINDEX, "ExtensionInfo");
    sip_client *cli =(sip_client *)lua_touserdata(L,-1);
    if(cli)
    {
        cli->send_msg(msg_);
        lua_yield(L, 0);
    }
    return 0;
}

net_poll::net_poll()
:epfd(0)
{
    epfd = epoll_create(102400);
    revents = new epoll_event[64];
}
bool net_poll::loop()
{
    while (true)
    {
        int num = epoll_wait(epfd, revents, 102400, 3 * 1000);
        for (int i = 0; i < num; i++)
        {
            if (revents[i].events & EPOLLIN)
            { 
                //read event
                sip_client *c = (sip_client *)(revents[i].data.ptr);
                if (c && c->on_read() == 0)
                {
                    del_client(c);
                }
            }
            else if(revents[i].events & EPOLLERR)
            {
                sip_client *c = (sip_client *)(revents[i].data.ptr);
                if(c) delete c; c = NULL;
            }
        }
    }
    return 0;
}
bool net_poll::add_client(sip_client *c)
{
    epoll_event event;
    event.events = EPOLLIN | EPOLLERR;
    event.data.ptr = c;
    epoll_ctl(epfd, EPOLL_CTL_ADD, c->connect->get_fd(), &event);
    return true;
}
bool net_poll::del_client(sip_client *c)
{
    cout<<"epoll del client \n";
    epoll_ctl(epfd, EPOLL_CTL_DEL, c->connect->get_fd(), NULL);
    if(c) delete c; c = NULL;
    return true;
}


sip_client::sip_client(Protocol type, std::string &ipaddr, int port, std::string &path, net_poll *np)
:connect(NULL),
buf(""),
L(NULL),
m_netpoll(np)
{
    if(type == T_TCP)
    {
        connect = new tcp_connnect(ipaddr, port);
    }
    else if(type == T_TLS)
    {
        connect = new tls_connnect(ipaddr, port);
    }
    L = luaL_newstate();
    lua_pushlightuserdata(L, this);
    lua_setfield(L, LUA_REGISTRYINDEX, "ExtensionInfo");
    lua_register(L, "log", log);
    lua_register(L, "sendmsg", sendmsg);
    luaL_loadfile(L, path.c_str());
}
sip_client::~sip_client()
{
    if(connect) delete connect;
}
bool sip_client::run_script()
{
    return resume_lua_state(L);
}
bool sip_client::on_sip_msg(const char *sip_msg)
{
    lua_pushstring(L, sip_msg);
    lua_setglobal(L, "SIP_MSG");
    return resume_lua_state(L);
}
bool sip_client::resume_lua_state(lua_State *L)
{
    int status = lua_resume(L, NULL, 0);
    if(status == LUA_ERRRUN)
    {
        cout<<" lua run script error , drop client "<< status << endl;
        //m_netpoll->del_client(this);
        return false;
    }
    return true;
}
int sip_client::on_read()
{
    char tmp_buf[1024] = {0};
    int ret = connect->on_read(tmp_buf, sizeof(tmp_buf));
    bool lua_run_status = true;
    if(ret > 0)
    {
        if(ret == 2)
        {
            lua_run_status = on_sip_msg("pong response");
        }
        else
        {
            buf.append(tmp_buf);
            auto pos = buf.find("\r\n\r\n");
            if( pos != std::string::npos)
            {
                string msg = buf.substr(0, pos);
                lua_run_status = on_sip_msg(msg.c_str());
                buf = buf.substr(pos + 4, buf.length());
            }
        }
    }
    if(lua_run_status)
    {
        cout<<" ret = "<< ret << endl;
        return ret;
    }
    else
    {
        cout<<"on read 0 \n";
        return 0;
    }
}
void sip_client::send_msg(std::string &msg)
{
    if(connect) connect->on_write(msg.c_str(), msg.length());
}