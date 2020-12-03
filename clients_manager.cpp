
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
bool clients_manager::del_client(shared_ptr<client> cli)
{
    spinlock_lock(&m_lock);
    auto pos = cli_map.find(cli->getfd());
    if(pos != cli_map.end())
    {
        cli_map.erase(pos);
    }
    spinlock_unlock(&m_lock);
    return true;
}
bool clients_manager::del_client(int fd)
{
    spinlock_lock(&m_lock);
    auto pos = cli_map.find(fd);
    if(pos != cli_map.end())
    {
        cli_map.erase(pos);
    }
    spinlock_unlock(&m_lock);
    return true;
}
bool clients_manager::is_exsist(client *cli)
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
shared_ptr<client> clients_manager::getclient(int fd)
{
    shared_ptr<client> cli;
    spinlock_lock(&m_lock);
    cli = cli_map[fd];
    spinlock_unlock(&m_lock);
    return cli;
}
bool clients_manager::add_client(shared_ptr<client> cli, net_poll *p)
{
    spinlock_lock(&m_lock);
    cli_map[cli->getfd()] = cli;
    p->epoll_add(cli->getfd());
    spinlock_unlock(&m_lock);
    return true;
}


// net_poll
net_poll::net_poll()
{
    epfd = epoll_create(10240);
}
bool net_poll::loop(int read_fd)
{
    // add event fd watch
    epoll_event event;
    event.events = EPOLLIN | EPOLLERR;
    event.data.fd = read_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, read_fd, &event);

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
                        shared_ptr<client> cli = clients_manager::instance()->getclient(fd);
                        if(cli)
                        {
                            cli->run_script(WAKE_TIMER);
                        }
                    }
                }
                else
                {
                    int fd = revents[i].data.fd;
                    shared_ptr<client> cli = clients_manager::instance()->getclient(fd);
                    if (cli)
                    {
                        if (cli->on_read() == 0)
                        {
                            epoll_del(fd);
                            clients_manager::instance()->del_client(fd);
                        }
                    }
                }
            }
            else if (revents[i].events & EPOLLERR)
            {
                cout << "read error notify \n";
                int fd = revents[i].data.fd;
                epoll_del(fd);
                clients_manager::instance()->del_client(fd);
            }
        }
    }
    return 0;
}
bool net_poll::epoll_add(int fd)
{
    epoll_event event;
    event.events = EPOLLIN | EPOLLERR;
    event.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
    return true;
}
bool net_poll::epoll_del(int fd)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    return true;
}
