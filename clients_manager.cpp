
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
    auto pos = cli_multiset.find(cli);
    if(pos != cli_multiset.end())
    {
        delete (sip_client *)(*pos);
        cli_multiset.erase(pos);
    }
    spinlock_unlock(&m_lock);
    return true;
}
bool clients_manager::is_exsist(sip_client *cli)
{
    bool ret = true;
    spinlock_lock(&m_lock);
    auto pos = cli_multiset.find(cli);
    if( pos != cli_multiset.end() )
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
bool clients_manager::add_client(sip_client *cli)
{
    spinlock_lock(&m_lock);
    cli_multiset.insert(cli);
    spinlock_unlock(&m_lock);
    return true;
}
void clients_manager::do_task()
{
    spinlock_lock(&m_lock);
    /*
    for(auto it = cli_multiset.rbegin(); it != cli_multiset.rend();++it)
    {
        sip_client *cli = (sip_client *)(*it);
        cout<<"do_task end time = "<< cli->end_time << endl;
    }
    */
    for(auto it = cli_multiset.rbegin(); it != cli_multiset.rend();++it )
    {
        sip_client *cli = (sip_client *)(*it);
        if( cli->end_time < time(NULL) )
        {
            cli->run_script();
        }
        else
        {
            break;
        }
    }
    spinlock_unlock(&m_lock);
}
void seconds_sleep(unsigned seconds){
    struct timeval tv;
    tv.tv_sec=seconds;
    tv.tv_usec=0;
    int err;
    do{
       err=select(0,NULL,NULL,NULL,&tv);
    }while(err<0 && errno==EINTR);
}
void clients_manager::loop(int write_fd)
{
    while(true)
    {
        spinlock_lock(&m_lock);
        if(!cli_multiset.empty())
        {
            auto it = cli_multiset.rbegin();
            sip_client *cli = (sip_client *)(*it);
            if( cli->end_time <= time(NULL) )
            {
                char buf[1];
                buf[0] = 'e';
                write(write_fd, buf, 1);
            }
        }
        spinlock_unlock(&m_lock);
        seconds_sleep(1);
    }
}