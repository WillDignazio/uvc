#include <string>
#include <cstring>
#include "uvc.hpp"

using namespace std;

const char* REQUEST_TEMPLATE =
  "GET /%s HTTP/1.1\n" \
  "Host: %s\n\n";

const string fill_req_template(const string &host, const string &name)
{
  int bufsize = strlen(REQUEST_TEMPLATE) - (2 * 2) + // replace chars
    name.length() + host.length() + 1;

  char buffer[bufsize];
  snprintf(buffer, sizeof(buffer), REQUEST_TEMPLATE, name.c_str(), host.c_str());

  return string(buffer);
}
