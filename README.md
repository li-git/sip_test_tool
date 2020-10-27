# sip test tool

*   lua 虚拟机模拟终端
*   脚本中收发网络数据，通过异步socket来驱动虚拟机的运行
*   class client里面可以通过重新on_read，解析数据来实现其他协议，这样在脚本中不用关系包解析
