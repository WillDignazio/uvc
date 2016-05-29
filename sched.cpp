#include <iostream>
#include <cstring>
#include <memory>

#include "uvc.hpp"

void event_loop(Scheduler *sched);
void worker_thr(Scheduler *sched);

Scheduler::Scheduler(const vector<shared_ptr<UVBSocket>> _sockets):
  sockets {_sockets},
  stopped {false}
{
  poll_fds = new struct pollfd[sizeof (struct pollfd) * sockets.size()];
  poll_fds_count = sockets.size();
  
  /* Initialize socket poll descriptors */
  for (auto si=sockets.begin(); si != sockets.end(); ++si) {
    int socketfd = (*si)->socket_fd();
    poll_fds[si - sockets.begin()] = {
      socketfd,
      POLLOUT,
      0
    };
  }

  //  int ncores = thread::hardware_concurrency();
  for (int ndx=0; ndx < 1; ++ndx) {
    thread *thr = new thread(worker_thr, this);
    worker_threads.push_back(thr);
  }
}

Scheduler::~Scheduler()
{
  cout << "Shut down scheduler" << endl;

  /* Wait up for threads to finish */
  stopped = true;
  signal.notify_all();
  this_thread::sleep_for(chrono::milliseconds(1000*3));
  
  if (poll_fds != nullptr)
    delete[] poll_fds;
}

thread* Scheduler::start()
{
  /* Last step, begin event loop */
  event_thread = thread(event_loop, this);
  return &event_thread;
}

int Scheduler::schedule(shared_ptr<ScheduleOp> opptr)
{
  shared_ptr<ScheduleOp> refptr = opptr;
  unique_lock<mutex> lock(sched_lock);
  
  op_queue.push(refptr);

  return 0;
}

bool Scheduler::is_stopped()
{
  return stopped;
}

void worker_thr(Scheduler *sched)
{
  while (!sched->is_stopped()) {

    if (sched->op_queue.empty()) {
      do {
	unique_lock<mutex> lock(sched->sched_lock);
	sched->signal.wait(lock);
      } while (sched->op_queue.empty());
      /* XXX Fallthrough: Got lock back after signal  */
    }

    unique_lock<mutex> lock(sched->sched_lock);
    
    shared_ptr<ScheduleOp> op = sched->op_queue.front();
    shared_ptr<UVBSocket> socket = get<2>(*op);
    
    sched->op_queue.pop();
    
    lock.unlock();

    switch (get<0>(*op)) {
    case READ:
      socket->recv_message();
      get<1>(*op)->events = POLLOUT;
      continue;
    case WRITE:
      socket->emit_payload();
      get<1>(*op)->events = POLLIN;
      continue;
    case SHUTDOWN:
    default:
      cerr << "I don't know what to do" << endl;
    }
  }
}

shared_ptr<ScheduleOp> process_event(struct pollfd *pfd, shared_ptr<UVBSocket> socket)
{
  shared_ptr<UVBSocket> sockptr = socket;

  switch (socket->socket_state()) {
  case READ:
  case WRITE:
    /*
     * We handle standard events by wiping the events we are interested in,
     * and returning an operation according to the state the socket was in.
     */
    pfd->events = 0;
    pfd->revents = 0;
    return make_shared<ScheduleOp>(socket->socket_state(), pfd, sockptr);
  case SHUTDOWN:
    cerr << "Shutting down socket..." << endl;
    return make_shared<ScheduleOp>(SHUTDOWN, pfd, sockptr);
  default:
    throw "Unknown event operation";
  }
}

/**
 * Main Scheduling Event Loop
 *
 * We poll over the group of file descriptors associated with the sockets given
 * at construction. The events are fed into a event queue, which workers then
 * read from.
 */
void event_loop(Scheduler *sched)
{
  for (;;) {
    int ret = poll(sched->poll_fds, sched->poll_fds_count, 10);
    if (ret < 1) {
      if (ret == 0) {
	cerr << "Timeout on poll..." << endl;
	continue;
      }

      cerr << "poll error: " << string(strerror(errno)) << endl;
      continue;
    }

    /* ret contains number of events, we don't know where */
    int pcnt = 0;

    unique_lock<mutex> lock(sched->sched_lock);
    for (unsigned int pidx=0; pidx < sched->poll_fds_count; ++pidx) {
      if (sched->poll_fds[pidx].revents == 0) // No Changes
	continue;

      shared_ptr<ScheduleOp> op = process_event(&sched->poll_fds[pidx],
						sched->sockets.at(pidx));

      lock.unlock();

      /* Lop the operation into the scheduler */
      ret = sched->schedule(op);
      if (ret != 0) {
	cerr << "Failed to schedule operation" << endl;
      }
      
      lock.lock();
      
      ++pcnt;
      if  (pcnt == ret) // Found all changed sockets
	break;
    }

    lock.unlock();
    sched->signal.notify_all();
  }
}
