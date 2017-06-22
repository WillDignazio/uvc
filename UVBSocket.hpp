#ifndef __UVBSOCKET__
#define __UVBSOCKET__

#include <string>
using std::string;

#include <netdb.h>

#include "uvc.hpp"

class UVBSocket
{
private:
    const string host;
    const string portstr;
    const string& payload;

    struct addrinfo host_info_in;
    struct addrinfo *host_info_ret;  
    int socketfd;
    opstate state;

    /* TODO: remove or share */
    char *buffer;
    long unsigned int buffersz;
  
    public:
    UVBSocket(const string& _host, const string& _portstr, const string& _payload);
    ~UVBSocket();
	      
    int emit_payload();
    int recv_message();

    int socket_fd();
    opstate socket_state();
};

#endif

