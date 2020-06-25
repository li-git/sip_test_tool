#include "sip_client.h"
#include "clients_manager.h"

int sendmsg(lua_State *L)
{
    int argc = lua_gettop(L);
    std::string msg_ = "";
    bool is_noblock = true;
    if(argc == 1)
    {
        msg_ = lua_tostring(L, -1);
    }
    else if(argc == 2)
    {
        msg_ = lua_tostring(L, -2);
        is_noblock = lua_toboolean(L, -1);
    }
    lua_getfield(L, LUA_REGISTRYINDEX, "ExtensionInfo");
    sip_client *cli = (sip_client *)lua_touserdata(L, -1);
    if (cli)
    {
        cli->send_msg(msg_);
        if(is_noblock)
        {
            lua_yield(L, 0);
        }
    }
    return 0;
}
int sleep_(lua_State *L)
{
    uint32_t sleep_delay = lua_tonumber(L, -1);
    lua_getfield(L, LUA_REGISTRYINDEX, "ExtensionInfo");
    sip_client *cli = (sip_client *)lua_touserdata(L, -1);
    if (cli)
    {
        cli->end_time = time(NULL) + sleep_delay;
        //cout<<" add end time = " << cli->end_time <<endl;
        lua_yield(L, 0);
    }
    return 0;
}

net_poll::net_poll()
    : epfd(0)
{
    epfd = epoll_create(10240);
}
bool net_poll::loop()
{
    //main thread
    int fds[2];
    pipe(fds);
    int read_fd = fds[0];
    int wirte_fd = fds[1];

    // add event fd watch
    epoll_event event;
    event.events = EPOLLIN | EPOLLERR;
    event.data.fd = read_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, read_fd, &event);

    auto notify_thread = [&] { clients_manager::instance()->loop(wirte_fd); };
    std::thread notify_th(notify_thread);
    notify_th.detach();

    epoll_event revents[10240];
    while (true)
    {
        int num = epoll_wait(epfd, revents, 10240, 3 * 1000);
        for (int i = 0; i < num; i++)
        {
            if (revents[i].events & EPOLLIN)
            {
                if (revents[i].data.fd == read_fd)
                {
                    // notify event
                    char buf[1];
                    int ret = read(read_fd, buf, 1);
                    if (ret)
                    {
                        clients_manager::instance()->do_task();
                    }
                }
                else
                {
                    int fd = revents[i].data.fd;
                    auto pos = cli_map.find(fd);
                    if (pos != cli_map.end())
                    {
                        sip_client *cli = pos->second;
                        //printf("=========client %p=============connect %p \n",cli,  cli->m_connect);
                        if (cli->on_read() == 0)
                        {
                            epoll_del(cli);
                        }
                    }
                }
            }
            else if (revents[i].events & EPOLLERR)
            {
                cout << "read error notify \n";
                int fd = revents[i].data.fd;
                auto pos = cli_map.find(fd);
                if (pos != cli_map.end())
                {
                    epoll_del(pos->second);
                }
            }
        }
    }
    return 0;
}
bool net_poll::epoll_add(sip_client *c)
{
    //cout<<" add epoll add fd = "<< c->getfd() << endl;
    epoll_event event;
    event.events = EPOLLIN | EPOLLERR;
    event.data.fd = c->getfd();
    epoll_ctl(epfd, EPOLL_CTL_ADD, c->getfd(), &event);

    cli_map[c->getfd()] = c;
    clients_manager::instance()->add_client(c);

    return true;
}
bool net_poll::epoll_del(sip_client *c)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, c->getfd(), NULL);
    auto pos = cli_map.find(c->getfd());
    if (pos != cli_map.end())
    {
        cli_map.erase(pos);
    }
    clients_manager::instance()->del_client(c);
    return true;
}

static const luaL_Reg siplib[] = {
    {"log", log},
    {"sendmsg", sendmsg},
    {"sleep", sleep_},
    {NULL, NULL}};
int luaopen_lib(lua_State *L)
{
    luaL_newlib(L, siplib);
    return 1;
}
int lua_init(sip_client *cli)
{
    cli->L = luaL_newstate();
    luaL_openlibs(cli->L);
    lua_pushlightuserdata(cli->L, cli);
    lua_setfield(cli->L, LUA_REGISTRYINDEX, "ExtensionInfo");
    luaL_requiref(cli->L, "sip", luaopen_lib, 1);
}

sip_client::sip_client(Protocol type, std::string &ipaddr, int port, std::string &path, net_poll *np)
    : m_connect(NULL),
      buf(""),
      L(NULL),
      end_time(0),
      m_netpoll(np),
      user_data(NULL)
{
    if (type == T_TCP)
    {
        m_connect = new tcp_connnect(ipaddr, port);
    }
    else if (type == T_TLS)
    {
        m_connect = new tls_connnect(ipaddr, port);
    }
    lua_init(this);
    luaL_loadfile(L, path.c_str());
}
sip_client::~sip_client()
{
    printf(" delete client %p \n", this);
    if (m_connect)
    {
        delete m_connect;
        m_connect = NULL;
    }
}
bool sip_client::run_script(int argc)
{
    if (time(NULL) >= end_time)
    {
        int status = lua_resume(L, NULL, argc);
        if (status == LUA_ERRRUN)
        {
            cout << " lua run script error , drop client " << status << endl;
            m_netpoll->epoll_del(this);
            return false;
        }
    }
    return true;
}
int sip_client::getfd()
{
    if (m_connect)
    {
        return m_connect->get_fd();
    }
}
bool sip_client::on_sip_msg(const char *sip_msg)
{
    lua_pushstring(L, sip_msg);
    return run_script(1);
}
static char tmp_buf[1024] = {0};
int sip_client::on_read()
{
    memset(tmp_buf, 0, sizeof(tmp_buf));
    int ret = 0;
    if (m_connect)
    {
        ret = m_connect->on_read(tmp_buf, sizeof(tmp_buf));
    }
    if (ret > 0)
    {
        if (ret == 2 && tmp_buf[0] == '\r' && tmp_buf[1] == '\n')
        {
            if (!on_sip_msg("pong response"))
                return 0;
        }
        else
        {
            buf.append(tmp_buf);
            auto pos = buf.find("\r\n\r\n");
            if (pos != std::string::npos)
            {
                string msg = buf.substr(0, pos);
                buf = buf.substr(pos + 4, buf.length());
                if (!on_sip_msg(msg.c_str()))
                    return 0;
            }
        }
    }
    return ret;
}
void sip_client::send_msg(std::string &msg)
{
    if (m_connect)
        m_connect->on_write(msg.c_str(), msg.length());
}
bool sip_client::operator<(const sip_client &c)
{
    return c.end_time < this->end_time;
}