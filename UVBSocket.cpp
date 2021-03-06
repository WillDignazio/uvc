#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <netinet/tcp.h>

#include <iostream>
using std::cerr;
using std::cout;
using std::endl;

#include "UVBSocket.hpp"
#include "uvc.hpp"

UVBSocket::UVBSocket(const string& _host,
		     const string& _portstr,
		     const string& _payload)
    : host {_host}
    , portstr {_portstr}
    , payload {_payload}
    , state {WRITE}
    , buffer {new char[_payload.size()]}
    , buffersz {_payload.size()}
  {
      memset(&host_info_in, 0, sizeof host_info_in);
      host_info_in.ai_family = AF_UNSPEC;
      host_info_in.ai_socktype = SOCK_STREAM;

      int status = getaddrinfo(host.c_str(), portstr.c_str(),
                               &host_info_in,
                               &host_info_ret);
      if (status != 0) {
          cerr << "Failed to getaddrinfo: " + string(strerror(errno)) << endl;
          throw "Failed to getaddrinfo";
      }
    
      socketfd = socket(host_info_ret->ai_family,
                        host_info_ret->ai_socktype,
                        host_info_ret->ai_protocol);

      if (socketfd == -1) {
          cerr << "Failed to open socket" + string(strerror(errno)) << endl;
          throw "Failed to open socket";
      }

      status = connect(socketfd,
                       host_info_ret->ai_addr,
                       host_info_ret->ai_addrlen);
      if (status != 0) {
          cerr << "Connection Error: " << strerror(errno) << endl;
          throw "Failed to connect socket";
      }

      /* Configure Socket */
      status = fcntl(socketfd, F_SETFL, O_NONBLOCK);
      if (status != 0)
          throw ("Failed to set socket as nonblocking: " + string(strerror(errno)));

      int flag=1;
      status = setsockopt(socketfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
      if (status < 0) {
          cerr << "Failed to disable nagle's algorithm: " + string(strerror(errno)) << endl;
          throw "Error configuring socket";
      }
  }

UVBSocket::~UVBSocket()
{
    int status = close(socketfd);
    if (status != 0) {
        cerr << "Destruction error: " << strerror(errno) << endl;
        throw "Failed to close socket";
    }

    if (host_info_ret != NULL)
        free(host_info_ret);

    delete[] buffer;
    cout << "Closed UVBSocket" << endl;
}

int UVBSocket::socket_fd()
{
    return socketfd;
}

opstate UVBSocket::socket_state()
{
    return state;
}

int UVBSocket::emit_payload()
{
    state = READ;
    int i = 1;
    setsockopt(socketfd, IPPROTO_TCP, TCP_QUICKACK, (void *)&i, sizeof(i));    
    return send(socketfd, payload.c_str(), payload.length(), MSG_EOR);
}

int UVBSocket::recv_message()
{
    state = WRITE;
    return recv(socketfd, buffer, buffersz, 0);
}
