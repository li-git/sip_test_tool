#ifndef __CLIENTS_MANAGER_H__
#define __CLIENTS_MANAGER_H__
#include "client.h"
#include "spin_lock.h"

class net_poll
{
public:
    net_poll();
    bool loop(int read_fd);
    bool epoll_add(client *c);
    bool epoll_del(client *c, int fd);
public:
    int epfd;
};

class clients_manager
{
public:
    clients_manager();
    static clients_manager *instance();
    bool del_client(client *cli);
    bool del_client(int fd);
    bool add_client(client *cli);
    bool is_exsist(client *cli);
    client *getclient(int fd);
    void loop(int write_fd);
public:
    std::map<int /*fd*/, client *> cli_map;
    spinlock m_lock;
};

#endif
