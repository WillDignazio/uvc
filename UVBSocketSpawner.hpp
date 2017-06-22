#ifndef __SPAWNER__
#define __SPAWNER__

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <memory>
using std::shared_ptr;

#include "UVBSocket.hpp"

class UVBSocketSpawner {
public:
    UVBSocketSpawner(int nworkers, const string& _host, const string& _portstr, const string& _payload);
    ~UVBSocketSpawner();
    
public:
    vector<shared_ptr<UVBSocket>> spawn(int nsockets) const;
    shared_ptr<UVBSocket> spawn() const;

private:
    const int nworkers;
    const string& host;
    const string& portstr;
    const string& payload;
};

#endif

