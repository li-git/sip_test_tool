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
    bool add_client(sip_client *cli);
    bool is_exsist(sip_client *cli);

    void do_task();
    void loop(int write_fd);
public:
    std::multiset<sip_client *> cli_multiset;
    spinlock m_lock;
};

#endif
