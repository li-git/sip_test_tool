#include "util.h"
#include <thread>
#include <mutex> 
#include <atomic>
#include <condition_variable>
#include <queue>
#include <mysql/mysql.h>
using namespace std;
class MysqlHandle
{
public:
	MysqlHandle(const char* user, const char* pswd, const char* host, const char* dbname, int port)
	{	
		bool reconnect = 1;
		mysql_init(&myCont);
		mysql_options(&myCont, MYSQL_OPT_RECONNECT, &reconnect);
		while(true)
		{
			if (mysql_real_connect(&myCont, host, user, pswd, dbname, port, NULL, 0))
			{
				mysql_query(&myCont, "SET NAMES UTF8");
				break;
			}
			else
			{
				printf("sql connect failed! \n");
				sleep(1);
			}
		}
	}
	~MysqlHandle()
	{
		mysql_close(&myCont);
	}
	int sql(string sql,lua_State *L)
	{
		int res = mysql_query(&myCont, sql.c_str());
		MYSQL_RES *result = NULL;
		if (!res && (result=mysql_store_result(&myCont)))
		{
			MYSQL_ROW row;
			unsigned int num_fields = mysql_num_fields(result);
			MYSQL_FIELD *fields = mysql_fetch_fields(result);
			
			int row_index = 1;
			lua_newtable(L);
			while ((row = mysql_fetch_row(result)))
			{
			   lua_newtable(L);
			   for(int i = 0; i < num_fields; i++)
			   {
			       //printf("%s=%s ",fields[i].name,  row[i] ? row[i] : "NULL");
			       lua_pushstring(L, fields[i].name);
			       lua_pushstring(L, row[i] ? row[i] : "NULL");
			       lua_rawset(L, -3);
			   }
			   lua_rawseti(L, -2, row_index);
			   ++row_index;
			}
			mysql_free_result(result);
			return 0;
		}
		else
		{
			printf("Excute err [%s] \n",mysql_error(&myCont)?mysql_error(&myCont):"NULL");
			return -1;
		}
	}
public:
    MYSQL myCont;
};
class mysqlPool
{
public:
	mysqlPool(string &username, string &passwd, string &ip, int port, string &dbname , uint32_t num)
	{	
		lock_guard<mutex> lck(m_QueLock);
		for(int i=0;i<num;++i)
		{
            m_handles.push(new MysqlHandle(username.c_str(), passwd.c_str(), ip.c_str(), dbname.c_str(), port));
		}
	}
	~mysqlPool()
	{
		lock_guard<mutex> lck(m_QueLock);
		while(!m_handles.empty())
		{
			delete m_handles.front();
			m_handles.pop();
		}
	}
	MysqlHandle *gethandle()
	{
		unique_lock<mutex> lck(m_QueLock);
		m_QueCond.wait(lck, [this]{ return !this->m_handles.empty(); });
		MysqlHandle *handle = m_handles.front();
		m_handles.pop();	
		lck.unlock();
		return handle;
	}
	void release_handle(MysqlHandle *handle)
	{
		lock_guard<mutex> lck(m_QueLock);
		m_handles.push(handle);
		m_QueCond.notify_one();
	}	
public:
	queue<MysqlHandle*> m_handles;
	condition_variable m_QueCond;
	mutex m_QueLock;
};
