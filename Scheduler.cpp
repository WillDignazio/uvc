#include <iostream>
using std::cerr;
using std::cout;
using std::endl;

#include <mutex>
using std::unique_lock;

#include <chrono>
#include <thread>

#include <tuple>
using std::get;

#include <cstring>
#include <memory>
using std::make_shared;

#include "Scheduler.hpp"

Scheduler::Scheduler(const vector<shared_ptr<UVBSocket>> _sockets, int nthreads)
    : sockets {_sockets}
    , stopped {false}
    , nthreads {nthreads}
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
}

Scheduler::~Scheduler()
{
    cout << "Shut down scheduler" << endl;

    /* Wait up for threads to finish */
    stopped = true;
    signal.notify_all();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000*3));
  
    if (poll_fds != nullptr)
        delete[] poll_fds;
}

void Scheduler::start()
{
    /* Last step, begin event loop */
    for (int idx=0; idx < nthreads; ++idx)
        event_threads.push_back(new thread(&Scheduler::routine, this));
    cout << "Started scheduler threads." << endl;
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

void Scheduler::routine()
{
    /* spawn event_loop thread */
    event_threads.push_back(new thread(&Scheduler::event_loop, this));
    
    while (!is_stopped()) {

        if (op_queue.empty()) {
            do {
                unique_lock<mutex> lock(sched_lock);
                signal.wait(lock);
            } while (op_queue.empty());
            /* XXX Fallthrough: Got lock back after signal  */
        }

        unique_lock<mutex> lock(sched_lock);
    
        shared_ptr<ScheduleOp> op = op_queue.front();
        shared_ptr<UVBSocket> socket = get<2>(*op);
    
        op_queue.pop();
    
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
void Scheduler::event_loop()
{
    for (;;) {
        int ret = poll(poll_fds, poll_fds_count, 100);
        if (ret < 1) {
            if (ret == 0) {
                continue;
            }

            cerr << "poll error: " << string(strerror(errno)) << endl;
            continue;
        }

        /* ret contains number of events, we don't know where */
        int pcnt = 0;

        unique_lock<mutex> lock(sched_lock);
        for (unsigned int pidx=0; pidx < poll_fds_count; ++pidx) {
            if (poll_fds[pidx].revents == 0) // No Changes
                continue;

            shared_ptr<ScheduleOp> op = process_event(&poll_fds[pidx],
                                                      sockets.at(pidx));

            lock.unlock();

            /* Lop the operation into the scheduler */
            ret = schedule(op);
            if (ret != 0) {
                cerr << "Failed to schedule operation" << endl;
            }
      
            lock.lock();
      
            ++pcnt;
            if  (pcnt == ret) // Found all changed sockets
                break;
        }

        lock.unlock();
        signal.notify_all();
    }
}
