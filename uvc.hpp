#ifndef __UVC__
#define __UVC__

#include <string>
#include <netdb.h>
#include <sys/socket.h>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>

using namespace std;

class UVBSocket
{
private:
  const string host;
  const string portstr;
  const string& payload;

  struct addrinfo host_info_in;
  struct addrinfo *host_info_ret;  
  int socketfd;

  /* TODO: remove or share */
  char buffer[100];
  
public:
  UVBSocket(const string _host, const string _portstr, const string& _payload);
  ~UVBSocket();
	      
  int emit_payload();
  int recv_message();

  int socket_fd();
};


/**
 * Scheduler class
 * sched.cpp
 */
class Scheduler
{
  enum sched_op {
    READ,
    WRITE
  };
  
private:
  const vector<UVBSocket*> sockets;
  const queue<shared_ptr<pair<sched_op, UVBSocket*>>> op_queue;

  mutex sched_lock;
  
public:
  Scheduler(const vector<UVBSocket*> _sockets);
  ~Scheduler();

  /* Scheduling Operations */
  void schedule(sched_op op, UVBSocket* socket);
  void suspend();
  void resume();
};

/* util.cpp */
const string fill_req_template(const string &host, const string &name);

#endif
