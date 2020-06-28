#include "util.h"
#include "connect_handle.h"
#include "sip_client.h"
#include "clients_manager.h"

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
    xxx -serverip <ip> -port <port> -luafile <path> -connections <cons> -protocol <tcp/tls>\n\
";
int main(int argc, char **argv)
{
    std::string server_addr = "10.100.125.116";
    std::string script_path = "./sip.lua";
    int connections = 0;
    int port = 0;
    Protocol pro = T_TCP;
    if(argc < 5)
    {
        cout << help << endl;
        return -1;
    }
    for(int i =0; i< argc ; ++i)
    {
        if(std::string(argv[i])=="-serverip" && i<argc-1)
        {
            server_addr = argv[i+1];
        }
        else if(std::string(argv[i])=="-port" && i<argc-1)
        {
            port = std::atoi(argv[i+1]);
        }
        else if(std::string(argv[i])=="-luafile" && i<argc-1)
        {
            script_path = argv[i+1];
        }
        else if(std::string(argv[i])=="-connections" && i<argc-1)
        {
            connections = std::atoi(argv[i+1]);
        }
        else if(std::string(argv[i])=="-protocol" && i<argc-1)
        {
            if(std::string(argv[i+1])=="tls")
            {
                pro = T_TLS;
            }
        }
    }
    printf("  server  %s:%d lua  %s  conns %d \n", server_addr.c_str(), port, script_path.c_str(), connections);

    init_envs();
    net_poll *net_poll_ = new net_poll();
    for(int i=0; i< connections; ++i)
    {
        sip_client *cli = new sip_client(pro, server_addr, port , script_path, net_poll_);
        if(cli->run_script())
        {
            net_poll_->epoll_add(cli);
        }
        usleep(100);
    }
    net_poll_->loop();
    return 0;
}
