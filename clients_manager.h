#ifndef __CLIENTS_MANAGER_H__
#define __CLIENTS_MANAGER_H__
#include "sip_client.h"
#include "spin_lock.h"

class clients_manager
{
public:
    clients_manager();
    static clients_manager *instance();
    bool del_client(sip_client *cli);
    bool del_client(int fd);
    bool add_client(sip_client *cli);
    bool is_exsist(sip_client *cli);
    sip_client *getclient(int fd);
    void loop(int write_fd);
public:
    std::map<int /*fd*/, sip_client *> cli_map;
    spinlock m_lock;
};

#endif
