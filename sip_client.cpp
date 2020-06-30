#include "sip_client.h"
#include "clients_manager.h"
#include "mysql.h"

std::atomic<mysqlPool*> g_mysqlpool(NULL);

int mysql(lua_State *L)
{
    int argc = lua_gettop(L);
    if( argc < 6)
    {
        lua_pushnil(L);
    }
    string username = lua_tostring(L, -6);
    string passwd = lua_tostring(L, -5);
    string ip = lua_tostring(L, -4);
    int port = lua_tonumber(L, -3);
    string dbname = lua_tostring(L, -2);
    string sql = lua_tostring(L, -1);
    MysqlHandle handle(username.c_str(), passwd.c_str(), ip.c_str(), dbname.c_str(), port);
    handle.sql(sql.c_str(), L);
    return 1;
}
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
        notify_timer::instance()->addTask(cli);
        //cout<<" add end time = " << cli->end_time <<endl;
        lua_yield(L, 0);
    }
    return 0;
}
int md5_(lua_State *L)
{
    string data_ = lua_tostring(L, -1);
    lua_pushstring(L, MD5(data_).c_str());
    return 1;
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

    auto notify_thread = [&] { notify_timer::instance()->loop(wirte_fd); };
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
                    int fd = 0;
                    int ret = read(read_fd, (void*)&fd, sizeof(fd));
                    if (ret )
                    {
                        sip_client *cli = clients_manager::instance()->getclient(fd);
                        if(cli)
                        {
                            cli->run_script();
                        }
                    }
                }
                else
                {
                    int fd = revents[i].data.fd;
                    sip_client *cli = clients_manager::instance()->getclient(fd);
                    if (cli)
                    {
                        if (cli->on_read() == 0)
                        {
                            epoll_del(cli, fd);
                        }
                    }
                }
            }
            else if (revents[i].events & EPOLLERR)
            {
                cout << "read error notify \n";
                int fd = revents[i].data.fd;
                epoll_del(NULL, fd);
            }
        }
    }
    return 0;
}
bool net_poll::epoll_add(sip_client *c)
{
    epoll_event event;
    event.events = EPOLLIN | EPOLLERR;
    event.data.fd = c->getfd();
    epoll_ctl(epfd, EPOLL_CTL_ADD, c->getfd(), &event);

    clients_manager::instance()->add_client(c);
    return true;
}
bool net_poll::epoll_del(sip_client *c, int fd)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    clients_manager::instance()->del_client(fd);
    return true;
}
static const luaL_Reg siplib[] = {
    {"sendmsg", sendmsg},
    {"sleep", sleep_},
    {"md5", md5_},
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

    lua_register(cli->L, "log", log);
    lua_register(cli->L, "mysql", mysql);
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
bool sip_client::inject_values(int index)
{
    if(L)
    {
        lua_pushnumber(L, index);
        lua_setglobal(L, "index");
    }
    return true;
}
bool sip_client::run_script(int argc)
{
    if (time(NULL) >= end_time)
    {
        int status = lua_resume(L, NULL, argc);
        if (status == LUA_ERRRUN)
        {
            cout << " lua run script error , drop client " << status << endl;
            cout << "Trace : " << lua_tostring(L, -1) << endl;
            m_netpoll->epoll_del(this, getfd());
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
                auto content_pos = msg.find("Content-Length:");
                if( content_pos != std::string::npos)
                {
                    int length = std::atoi( msg.substr(content_pos, msg.length()).c_str());
                    if( length = 0 )
                    {
                        buf = buf.substr(pos + 4, buf.length());
                        if (!on_sip_msg(msg.c_str())) return 0;
                    }
                    else if( buf.length()-pos-4 >= length )
                    {
                        msg = buf.substr(0, pos + 4 + length);
                        buf = buf.substr(pos + 4 + length , buf.length());
                        if (!on_sip_msg(msg.c_str())) return 0;
                    }
                    else
                    {
                        // need read more buffer
                        return 1;
                    }
                }
            }
        }
    }
    return ret;
}
void sip_client::send_msg(std::string &msg)
{
    if (m_connect)
    {
        //cout << "send msg :" << msg;
        m_connect->on_write(msg.c_str(), msg.length());
    }
}
bool sip_client::operator<(const sip_client &c)
{
    return c.end_time < this->end_time;
}