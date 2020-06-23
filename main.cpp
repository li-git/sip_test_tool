#include "util.h"
#include "connect_handle.h"
#include "sip_client.h"


void init_envs()
{
    OpenSSL_add_all_algorithms();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
    SSL_load_error_strings();
    SSL_library_init();
}
int main(int argc, char **argv)
{
    init_envs();
    net_poll *net_poll_ = new net_poll();

    std::string server_addr = "10.100.126.241";
    std::string script_path = "./sip.lua";
    sip_client *cli = new sip_client(T_TCP, server_addr, 5090, script_path, net_poll_);
    if(!cli->run_script())
        net_poll_->del_client(cli);
    else
        net_poll_->add_client(cli);

    net_poll_->loop();

    pause();
    return 0;
}
