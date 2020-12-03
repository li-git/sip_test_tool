#include "client.h"
#include "mysql.h"

std::atomic<mysqlPool*> g_mysqlpool(NULL);
std::mutex mysql_mutex;

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
    mysql_mutex.lock();
    MysqlHandle handle(username.c_str(), passwd.c_str(), ip.c_str(), dbname.c_str(), port);
    mysql_mutex.unlock();
    handle.sql(sql.c_str(), L);
    return 1;
}
int sendmsg(lua_State *L)
{
    int argc = lua_gettop(L);
    std::string msg_ =  lua_tostring(L, -1);
    lua_getfield(L, LUA_REGISTRYINDEX, "ExtensionInfo");
    client *cli = (client *)lua_touserdata(L, -1);
    if (cli)
    {
        cli->send_msg(msg_);
    }
    return 0;
}
int sleep_(lua_State *L)
{
    uint32_t sleep_delay = lua_tonumber(L, -1);
    lua_getfield(L, LUA_REGISTRYINDEX, "ExtensionInfo");
    client *cli = (client *)lua_touserdata(L, -1);
    if (cli)
    {
        cli->isSleeping = true;
        timer::instance()->addTask(cli->getfd(), cli->m_notifyfd, sleep_delay);
        //cout<<" add end time = " << cli->end_time <<endl;
        //lua_yield(L, 0);
    }
    return 0;
}
int local_logs(lua_State *L)
{
    const char *log = lua_tostring(L, -1);
    time_t now_time = time(NULL);
    char buf[128] = {0};
    tm *local = localtime(&now_time);
    strftime(buf, 128, "%Y-%m-%d %H:%M:%S", local);

    lua_getfield(L, LUA_REGISTRYINDEX, "ExtensionInfo");
    client *cli = (client *)lua_touserdata(L, -1);

    std::ostringstream oss;
    oss << std::this_thread::get_id();
    std::cout << cli << "|" << buf << oss.str() << " ====> " << log  << std::endl;

    return 0;
}
int md5_(lua_State *L)
{
    string data_ = lua_tostring(L, -1);
    lua_pushstring(L, MD5(data_).c_str());
    return 1;
}
static const luaL_Reg ttlib[] = {
    {"sendmsg", sendmsg},
    {"sleep", sleep_},
    {"md5", md5_},
    {NULL, NULL}};
int luaopen_lib(lua_State *L)
{
    luaL_newlib(L, ttlib);
    return 1;
}
int lua_init(client *cli)
{
    cli->L = luaL_newstate();
    luaL_openlibs(cli->L);
    lua_pushlightuserdata(cli->L, cli);
    lua_setfield(cli->L, LUA_REGISTRYINDEX, "ExtensionInfo");
    luaL_requiref(cli->L, "tt", luaopen_lib, 1);

    lua_register(cli->L, "log", local_logs);
    lua_register(cli->L, "mysql", mysql);
}

client::client(Protocol type, std::string &ipaddr, int port, std::string &path)
    : m_connect(NULL),
      L(NULL),
      end_time(0),
      m_notifyfd(0),
      user_data(NULL),
      isSleeping(false)
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
    //luaL_loadfile(L, path.c_str());
    luaL_dofile(L, path.c_str());
}
client::~client()
{
    printf(" delete client %p \n", this);
    if (m_connect)
    {
        delete m_connect;
        m_connect = NULL;
    }
}
bool client::inject_values(int index)
{
    if(L)
    {
        lua_pushnumber(L, index);
        lua_setglobal(L, "index");
    }
    return true;
}
static int traceback(lua_State * L)
{
	lua_getglobal(L, "debug");
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return 1;
	}
	lua_getfield(L, -1, "traceback");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		return 1;
	}
	lua_pushvalue(L, 1);
	lua_pushinteger(L, 2);
	lua_call(L, 2, 1);
	return 1;
}
int docall(lua_State * L, int narg, int nresults, int perror, int fatal)
{
	int status;
	int base = lua_gettop(L) - narg;

	lua_pushcfunction(L, traceback);
	lua_insert(L, base);		

	status = lua_pcall(L, narg, nresults, base);

	lua_remove(L, base);
	if (status != 0) {
		lua_gc(L, LUA_GCCOLLECT, 0);
	}

	if (status && perror) {
		const char *err = lua_tostring(L, -1);
		if (err) {
			printf("%s\n", err);
		}
		if (fatal) {
			lua_error(L);
		} else {
			lua_pop(L, 1);
		}
	}

	return status;
}
bool client::run_script(WakeType argc)
{
    if( argc == WAKE_TIMER )
    {
        lua_getglobal(L, "luarunFun");
        lua_pushstring(L, "WAKE_TIMER");
        lua_pushnil(L);
        isSleeping = false;
    }
    else if( argc == WAKE_START )
    {
        lua_getglobal(L, "luarunFun");
        lua_pushstring(L, "WAKE_START");
        lua_pushnil(L);
    }
    else if(argc == WAKE_DATA && !isSleeping)
    {
        string argStr = lua_tostring(L, -1)?lua_tostring(L, -1):"";
        lua_pop(L,1);
        lua_getglobal(L, "luarunFun");
        lua_pushstring(L, "WAKE_DATA");
        lua_pushstring(L, argStr.c_str());
    }
    else
    {
        cout << "run_script type error argc" << argc << endl;
        return false;
    }
    if(docall(L, 2, 0, 0, 1)!= 0)
    {
        cout << " lua run script error , drop client \n" << string(lua_tostring(L, -1)) << endl;
        return false;
    }
    return true;
}
int client::getfd()
{
    if (m_connect)
    {
        return m_connect->get_fd();
    }
}
bool client::on_sip_msg(const char *sip_msg)
{
    lua_pushstring(L, sip_msg);
    return run_script(WAKE_DATA);
}
void client::send_msg(std::string &msg)
{
    if (m_connect)
    {
        //cout << "send msg :" << msg;
        m_connect->on_write(msg.c_str(), msg.length());
    }
}
bool client::operator<(const client &c)
{
    return c.end_time < this->end_time;
}

// for sip message
int sip_client::on_read()
{
    char tmp_buf[1024] = {0};
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
// for http client
int http_client::on_read()
{
    char tmp_buf[1024] = {0};
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