#include "timer.h"

timer *timer::instance()
{
    static timer n_timer;
    return &n_timer;
}
timer::timer()
{
    spinlock_init(&m_lock);
}
bool timer::addTask(int fd, int delay)
{
    spinlock_lock(&m_lock);
    Task task = Task(fd, delay);
    cli_multiset.insert(std::move(task));
    spinlock_unlock(&m_lock);
    return true;
}
void timer::loop(int notify_fd)
{
    prctl(PR_SET_NAME,"timer");
    while(true)
    {
        spinlock_lock(&m_lock);
        if(!cli_multiset.empty())
        {
            for(auto it = cli_multiset.rbegin(); it != cli_multiset.rend(); )
            {
                Task task = std::move(*it);
                if( task.m_time_stamp <= time(NULL) )
                {
                    int buf = task.m_fd;
                    write(notify_fd, (void *)&buf, sizeof(buf));
                    it = std::multiset< Task >::reverse_iterator( cli_multiset.erase((++it).base()) );
                }
                else
                {
                    //std::cout<<"timestamp = "<<task.m_time_stamp << std::endl;
                    //++it;
                    break;
                }
            }
        }
        spinlock_unlock(&m_lock);
        ms_sleep(100);
    }
}