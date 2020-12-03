#ifndef __CLIENTS_MANAGER_H__
#define __CLIENTS_MANAGER_H__
#include "client.h"
#include "spin_lock.h"

class net_poll
{
public:
    net_poll();
    bool loop(int read_fd);
    bool epoll_add(int fd);
    bool epoll_del(int fd);
public:
    int epfd;
};

class clients_manager
{
public:
    clients_manager();
    static clients_manager *instance();
    bool del_client(shared_ptr<client> cli);
    bool del_client(int fd);
    bool add_client(shared_ptr<client> cli, net_poll *p);
    bool is_exsist(client *cli);
    shared_ptr<client> getclient(int fd);
    void loop(int write_fd);
public:
    std::map<int /*fd*/, shared_ptr<client> > cli_map;
    spinlock m_lock;
};

#endif
