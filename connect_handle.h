#ifndef __CONNECT_HANDLE_H__
#define __CONNECT_HANDLE_H__
#include "util.h"
using namespace std;
class connect_base
{
public:
    connect_base() {}
    virtual ~connect_base(){}

    virtual int on_read(const char *msg, uint32_t lens) = 0;
    virtual int on_write(const char *msg, uint32_t lens) = 0;
    virtual int get_fd() = 0;
    virtual void reset() = 0;
};

class tcp_connnect : public connect_base
{
public:
    tcp_connnect(std::string &ipaddr, int port)
        : sock_fd(0),
          connected(false)
    {
        create_socket(ipaddr, port);
    }
    ~tcp_connnect()
    {
        cout << "disconnect socket " << sock_fd << endl;
        if (sock_fd)
            close(sock_fd);
    }
    int connect_timeout_(struct sockaddr_in *dst, int nsec)
    {
        int flags, n, error;
        socklen_t len;
        fd_set rset, wset;
        struct timeval tval;
        tval.tv_sec = nsec;
        tval.tv_usec = 0;
        error = 0;
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if ((flags = fcntl(sockfd, F_GETFL, 0)) == -1)
        {
            cout << "fcntl F_GETFL" << endl;
        }
        if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
        {
            cout << "fcntl F_SETFL" << endl;
        }
        if ((n = connect(sockfd, (struct sockaddr *)dst, sizeof(struct sockaddr_in))) < 0)
        {
            if (errno != EINPROGRESS)
            {
                cout << " connect failed " << endl;
                return -1;
            }
        }
        else if (n == 0)
        {
            goto done;
        }
        FD_ZERO(&rset);
        FD_SET(sockfd, &rset);
        wset = rset;
        if ((n = select(sockfd + 1, &rset, &wset, NULL, &tval)) == 0)
        {
            cout << " connect time out " << endl;
            close(sockfd);
            errno = ETIMEDOUT;
            return -1;
        }
        else if (n == -1)
        {
            close(sockfd);
            return -1;
        }
        if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset))
        {
            len = sizeof(error);
            if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
            {
                return -1;
            }
        }
        else
        {
            cout << "select error: socket not set" << endl;
        }
    done:
        fcntl(sockfd, F_SETFL, flags);
        if (error)
        {
            cout << "close fd  error " << strerror(error) << endl;
            close(sockfd);
            errno = error;
            return -1;
        }
        return 0;
    }
    int create_socket(std::string &hostname, int port)
    {
        char *tmp_ptr = NULL;
        struct sockaddr_in dest_addr;
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(port);
        dest_addr.sin_addr.s_addr = inet_addr(hostname.c_str());
        if (connect(sock_fd, (sockaddr *)&dest_addr, sizeof(sockaddr)) == -1)
        {
            cout << "Error: Cannot connect to host \n";
        }
        else
        {
            //cout << "tcp connect success \n";
            connected = true;
        }
    }
    virtual int on_read(const char *msg, uint32_t lens)
    {
        return read(sock_fd, (void *)msg, lens);
    }
    virtual int on_write(const char *msg, uint32_t lens)
    {
        return write(sock_fd, msg, lens);
    }
    virtual int get_fd()
    {
        return sock_fd;
    }
    void reset()
    {
        if(sock_fd)
        {
            close(sock_fd);
            sock_fd = 0;
        }
    }
public:
    int sock_fd;
    bool connected;
};
class tls_connnect : public tcp_connnect
{
public:
    tls_connnect(std::string &ipaddr, int port)
        : tcp_connnect(ipaddr, port)
    {
        cert = NULL;
        method = NULL;
        ctx = NULL;
        ssl = NULL;
        start_connect();
    }
    ~tls_connnect()
    {
        if (ssl)
            SSL_free(ssl);
        if (cert)
            X509_free(cert);
        if (ctx)
            SSL_CTX_free(ctx);
    }
    virtual int on_read(const char *buf, uint32_t len)
    {
        return SSL_read(ssl, (void *)buf, len);
    }
    virtual int on_write(const char *buf, uint32_t len)
    {
        return SSL_write(ssl, buf, len);
    }
    bool start_connect()
    {
        if (sock_fd == 0)
        {
            cout << " made the TCP connection failed .\n";
            return false;
        }
        method = SSLv23_client_method();
        if ((ctx = SSL_CTX_new(method)) == NULL)
            cout << "Unable to create a new SSL context structure.\n";

        SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);
        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, sock_fd);
        if (SSL_connect(ssl) != 1)
        {
            cout << "Error: Could not build a SSL session .\n";
            connected = false;
        }
        else
            cout << " SSL connect success \n";
        cert = SSL_get_peer_certificate(ssl);
        if (cert == NULL)
            cout << "Error: Could not get a certificate.\n";
        return true;
    }

public:
    X509 *cert;
    const SSL_METHOD *method;
    SSL_CTX *ctx;
    SSL *ssl;
};

#endif //__CONNECT_HANDLE_H__
