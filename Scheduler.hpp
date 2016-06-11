#ifndef __SCHEDULER__
#define __SCHEDULER__

#include <tuple>
using std::tuple;

#include <memory>
using std::shared_ptr;

#include <thread>
using std::thread;

#include <vector>
using std::vector;

#include <queue>
using std::queue;

#include <condition_variable>
using std::condition_variable;

#include <mutex>
using std::mutex;

#include <poll.h>

#include "UVBSocket.hpp"
#include "uvc.hpp"

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

#endif
