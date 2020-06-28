
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
    notify_timer::instance()->delTask(cli);
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
    notify_timer::instance()->delTask(cli);
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
notify_timer *notify_timer::instance()
{
    static notify_timer n_timer;
    return &n_timer;
}
notify_timer::notify_timer()
{
    spinlock_init(&m_lock);
}
bool notify_timer::addTask(sip_client *cli)
{
    if(cli)
    {
        spinlock_lock(&m_lock);
        cli_multiset.insert(cli);
        spinlock_unlock(&m_lock);
    }
    return true;
}
bool notify_timer::delTask(sip_client *cli)
{
    spinlock_lock(&m_lock);
    auto pos = cli_multiset.find(cli);
    if( pos != cli_multiset.end() )
    {
        cli_multiset.erase(cli);
    }
    spinlock_unlock(&m_lock);
    return true;
}
void notify_timer::loop(int notify_fd)
{
    while(true)
    {
        spinlock_lock(&m_lock);
        if(!cli_multiset.empty())
        {
            for(auto it = cli_multiset.rbegin(); it != cli_multiset.rend(); )
            {
                sip_client *cli = (sip_client *)(*it);
                if(cli->end_time && cli->end_time < time(NULL) )
                {
                    int buf = cli->getfd();
                    cli->end_time = 0;
                    write(notify_fd, (void *)&buf, sizeof(buf));
                    it = std::multiset<sip_client *>::reverse_iterator( cli_multiset.erase((++it).base()) );
                }
                else
                {
                    ++it;
                    break;
                }
            }
        }
        spinlock_unlock(&m_lock);
        ms_sleep(100);
    }
}