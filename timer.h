#ifndef __CLIENTS_TIMER_H__
#define __CLIENTS_TIMER_H__
#include "util.h"
#include "spin_lock.h"

class Task
{
public:
    Task(int fd, int delay)
    {
        m_fd = fd;
        m_time_stamp = time(NULL) + delay;
    }
    ~Task(){}
    friend bool operator<(const Task &c, const Task &b)
    {
        return c.m_time_stamp > b.m_time_stamp;
    }
public:
    int m_fd;
    int m_time_stamp;
};
class timer
{
public:
    timer();
    ~timer(){}
    static timer *instance();
    bool addTask(int fd, int delay);
    void loop(int notify_fd);
public:
    std::multiset< Task > cli_multiset;
    spinlock m_lock;
};

#endif
