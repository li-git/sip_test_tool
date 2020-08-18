
#include "clients_manager.h"

clients_manager cli_mg;

clients_manager::clients_manager()
{
    spinlock_init(&m_lock);
}
clients_manager *clients_manager::instance()
{
    return &cli_mg;
}
bool clients_manager::del_client(sip_client *cli)
{
    spinlock_lock(&m_lock);
    auto pos = cli_map.find(cli->getfd());
    if(pos != cli_map.end())
    {
        delete (sip_client *)(pos->second);
        cli_map.erase(pos);
    }
    spinlock_unlock(&m_lock);
    return true;
}
bool clients_manager::del_client(int fd)
{
    spinlock_lock(&m_lock);
    sip_client *cli = NULL;
    auto pos = cli_map.find(fd);
    if(pos != cli_map.end())
    {
        cli = (sip_client *)(pos->second);
        delete (sip_client *)(pos->second);
        cli_map.erase(pos);
    }
    spinlock_unlock(&m_lock);
    return true;
}
bool clients_manager::is_exsist(sip_client *cli)
{
    bool ret = true;
    spinlock_lock(&m_lock);
    auto pos = cli_map.find(cli->getfd());
    if( pos != cli_map.end() )
    {
        ret = true;
    }
    else
    {
        ret = false;
    }
    spinlock_unlock(&m_lock);
    return ret;
}
sip_client *clients_manager::getclient(int fd)
{
    spinlock_lock(&m_lock);
    sip_client *cli = NULL;
    auto pos = cli_map.find(fd);
    if( pos != cli_map.end() )
    {
        cli  = (sip_client *)(pos->second);
    }
    spinlock_unlock(&m_lock);
    return cli;
}
bool clients_manager::add_client(sip_client *cli)
{
    spinlock_lock(&m_lock);
    cli_map[cli->getfd()] = cli;
    spinlock_unlock(&m_lock);
    return true;
}
