#ifndef __UVC__
#define __UVC__

#include <string>
using std::string;

/* util.cpp */
const string fill_req_template(const string &host, const string &name);

enum opstate {
  READ,
  WRITE,
  SHUTDOWN
};

#endif

