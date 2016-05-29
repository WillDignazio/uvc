#ifndef __UVC__
#define __UVC__

#include <string>
#include <netdb.h>
#include <sys/socket.h>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include <thread>
#include <poll.h>
#include <condition_variable>

using namespace std;

enum opstate {
  READ,
  WRITE,
  SHUTDOWN
};

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
  
public:
  UVBSocket(const string _host, const string _portstr, const string& _payload);
  ~UVBSocket();
	      
  int emit_payload();
  int recv_message();

  int socket_fd();
  opstate socket_state();
};


/**
 * Scheduler class
 * sched.cpp
 */
using ScheduleOp = tuple<opstate, struct pollfd*, shared_ptr<UVBSocket>>;
class Scheduler
{ 
private:  
  thread event_thread;
  
public:
  const vector<shared_ptr<UVBSocket>> sockets;
  vector<thread*> worker_threads;
  queue<shared_ptr<ScheduleOp>> op_queue;
  
  struct pollfd *poll_fds;
  nfds_t poll_fds_count;

  bool stopped;
  condition_variable signal;
  mutex sched_lock; 
  
  Scheduler(const vector<shared_ptr<UVBSocket>> _sockets);
  ~Scheduler();

  /* Scheduling Operations */
  int schedule(shared_ptr<ScheduleOp> op);
  void suspend();
  void resume();
  thread* start();

  bool is_stopped();
};

/* util.cpp */
const string fill_req_template(const string &host, const string &name);

#endif
