#include <vector>
using std::vector;

#include <thread>
using std::thread;

#include <atomic>
using std::atomic_int_fast64_t;

#include <mutex>
using std::mutex;
using std::lock_guard;

#include <memory>
using std::shared_ptr;
using std::make_shared;

#include <iostream>
using std::endl;
using std::cout;

#include <iomanip>
using std::setw;

#include "UVBSocketSpawner.hpp"
#include "UVBSocket.hpp"

UVBSocketSpawner::UVBSocketSpawner(int nworkers, const string& _host, const string& _portstr, const string& _payload)
    : nworkers{nworkers}
    , host{_host}
    , portstr{_portstr}
    , payload{_payload} {    
}

UVBSocketSpawner::~UVBSocketSpawner() {
}

shared_ptr<UVBSocket> UVBSocketSpawner::spawn() const {
    return make_shared<UVBSocket>(host, portstr, payload);
}

vector<shared_ptr<UVBSocket>> UVBSocketSpawner::spawn(int nsockets) const {
    vector<thread> threads{};
    vector<shared_ptr<UVBSocket>> sockets{};
    mutex lock;
        
    atomic_int_fast64_t counter{0};
    for (int idx=0; idx < nworkers; ++idx) {
        threads.push_back(thread([&]() {
                    for (;;) {
                        atomic_int_fast64_t current{counter.fetch_add(1, std::memory_order_relaxed)};
                        if (current >= nsockets) {
                            break;
                        }

                        shared_ptr<UVBSocket> socket = spawn();
                        
                        lock_guard<mutex> guard(lock);
                        sockets.push_back(socket);

                        cout << setw(4) << "\r"
                             << "Spawning sockets... "
                             << (current / static_cast<double>(nsockets)) << "%"
                             << " (" << current  << "/" << nsockets << ")";
                    }
                }));
    }

    /* Wait until they are all done */
    for (thread& thr : threads)
        thr.join();

    cout << "Finished spawning sockets." << endl;
    return sockets;
}


