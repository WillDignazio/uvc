#include <iostream>
#include <argp.h>
#include <thread>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <chrono>

#include <poll.h>
#include "uvc.hpp"

using namespace std;

char const *DEFAULT_PORT_STR = "80";
const int ARGC_COUNT = 2;
const char *uvc_program_version		= "uvc v0.1";
const char *uvc_program_bug_address	= "<wdignazio@gmail.com>";
const char doc[] = "Ultimate Victory Battle Client";

static char args_doc[] = "URL NAME";

static struct argp_option options[] =
  {
    { "port", 'p', "PORT", 0, "Set port for UVB endpoint" },
    { 0 }
  };

struct arguments
{
  char *args[2];
  char const *portstr;
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  int portnum;
  struct arguments *arguments = (struct arguments*)state->input;
  
  switch (key) {
  case 'p':
    arguments->portstr = arg;
    break;
    
  case ARGP_KEY_ARG:
    if (state->arg_num >= ARGC_COUNT) {
      argp_usage(state);
      exit(1);
    }

    arguments->args[state->arg_num]= arg;
    break;

  case ARGP_KEY_END:
    if (state->arg_num < ARGC_COUNT) {
      argp_usage(state);
      exit(1);
    }
    break;

  default:
    return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

static struct argp argp = {
  options, parse_opt,
  args_doc,
  doc
};

int main(int argc, char *argv[])
{
  /* Parse Arguments */
  struct arguments arguments;
  memset(&arguments, 0, sizeof (struct arguments));
  arguments.portstr = DEFAULT_PORT_STR;
  
  argp_parse(&argp, argc, argv, 0, 0, &arguments);
  
  cout << "Starting uvc..." << endl;
  cout << "host: " << arguments.args[0] << endl;
  cout << "port: " << arguments.portstr << endl;
  cout << "name: " << arguments.args[1] << endl;

  const string payload = fill_req_template(arguments.args[0], arguments.args[1]);

  cout << "Payload:" << endl;
  cout << payload << endl;

  try {
    UVBSocket sock(string(arguments.args[0]),
		   string(arguments.portstr),
		   payload);

    /* Setup poll... */
    struct pollfd pfd = { sock.socket_fd(), POLLIN, 0 };
    auto ts = chrono::high_resolution_clock::now();    

    for (int idx=0; idx < 1000; ++idx) {
      sock.emit_payload();
      
      auto ts1 = chrono::high_resolution_clock::now();
      int res = poll(&pfd,1, 1000 * 100);
      if (res < 1) {
	cerr << "poll failure: " << string(strerror(errno)) << endl;
      }
      auto te1 = chrono::high_resolution_clock::now();
      cout << "Poll: " << chrono::duration_cast<chrono::milliseconds>(te1 - ts1).count() << "ms" << endl;
      
      sock.recv_message();
    }
    auto te = chrono::high_resolution_clock::now();
    cout << "Took: " << chrono::duration_cast<chrono::milliseconds>(te - ts).count() << "ms" << endl;
    
  } catch (const exception &exc) {
    cerr << "ERROR: "  << exc.what() << endl;
  }

  return 0;
}
