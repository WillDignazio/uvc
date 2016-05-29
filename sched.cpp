#include "uvc.hpp"

Scheduler::Scheduler(const vector<UVBSocket*> _sockets):
  sockets {_sockets}
{
}

Scheduler::~Scheduler()
{
}

