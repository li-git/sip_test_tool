#include "util.h"
#include "connect_handle.h"
#include "client.h"
#include "clients_manager.h"
#include "timer.h"

void init_envs()
{
    OpenSSL_add_all_algorithms();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
    SSL_load_error_strings();
    SSL_library_init();
}
const char* help = "\n\
Useage:\n\
    xxx -s <ip> -p <port> -lua <path> -c <cons> -pr <tcp/tls> -t <thread num>\n\
";
int main(int argc, char **argv)
{
    std::string server_addr = "10.100.125.116";
    std::string script_path = "./sip.lua";
    int connections = 0;
    int port = 0;
    int threads = 1;
    Protocol pro = T_TCP;
    if(argc < 5)
    {
        cout << help << endl;
        return -1;
    }
    for(int i =0; i< argc ; ++i)
    {
        if(std::string(argv[i])=="-s" && i<argc-1)
        {
            server_addr = argv[i+1];
        }
        else if(std::string(argv[i])=="-p" && i<argc-1)
        {
            port = std::atoi(argv[i+1]);
        }
        else if(std::string(argv[i])=="-luae" && i<argc-1)
        {
            script_path = argv[i+1];
        }
        else if(std::string(argv[i])=="-c" && i<argc-1)
        {
            connections = std::atoi(argv[i+1]);
        }
        else if(std::string(argv[i])=="-pr" && i<argc-1)
        {
            if(std::string(argv[i+1])=="tls")
            {
                pro = T_TLS;
            }
        }
        else if(std::string(argv[i])=="-t" && i<argc-1)
        {
            threads = std::atoi(argv[i+1]);
        }
    }
    printf("  server  %s:%d lua  %s  conns %d \n", server_addr.c_str(), port, script_path.c_str(), connections);

    init_envs();

    //timer thread
    auto notify_thread = [] { timer::instance()->loop(); };
    std::thread notify_th(notify_thread);
    notify_th.detach();

    //task threads
    auto task_thread = [&] {
        prctl(PR_SET_NAME,"task");
        int fds[2];
        pipe(fds);
        int read_fd = fds[0];
        int wirte_fd = fds[1];
        net_poll net_poll_;
        for(int i=0; i< connections; ++i)
        {
            shared_ptr<client> cli = shared_ptr<client>(new sip_client(pro, server_addr, port , script_path));
            cli->inject_values(i);
            cli->m_notifyfd = wirte_fd;
            if(cli->run_script(WAKE_START))
            {
                clients_manager::instance()->add_client(cli, &net_poll_);
            }
        }
        net_poll_.loop(read_fd);
    };

    std::vector< std::shared_ptr<std::thread> > tasks;
    for(int j = 0;j < threads;++j)
    {
        auto task = std::make_shared<std::thread>(std::thread(task_thread));
        task->detach();
        tasks.push_back(task);
    }
    while(true)
    {
        sleep(3);
    }
    return 0;
}
