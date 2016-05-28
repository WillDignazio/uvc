#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <argp.h>
#include <thread>
#include <vector>

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

const char* REQUEST_TEMPLATE =
  "GET /%s HTTP/1.1\n" \
  "Host: %s\n\n";

const string fill_req_template(const char *host, const char *name)
{
  if (name == nullptr)
    throw "Invalid pointer to host or name";
      
  int bufsize = strlen(REQUEST_TEMPLATE) - (2) + // replace chars
    strlen(name) + strlen(host) + 1;

  char buffer[bufsize];
  snprintf(buffer, sizeof(buffer), REQUEST_TEMPLATE, name, host);

  return string(buffer);
}

class Endpoint
{
private:
  const string host;
  const string name;
  const string portstr;

  string reqstr;
  struct addrinfo host_info_in;
  struct addrinfo *host_info_ret;  
  int socketfd;
  
public:
  Endpoint(const string _host, const string _portstr, const string _name):
    host {_host},
    portstr {_portstr},
    name {_name}
  {
    reqstr = fill_req_template(host.c_str(), name.c_str());
    
    memset(&host_info_in, 0, sizeof host_info_in);
    host_info_in.ai_family = AF_UNSPEC;
    host_info_in.ai_socktype = SOCK_STREAM;
    
    int status = getaddrinfo(host.c_str(), portstr.c_str(),
			     &host_info_in,
			     &host_info_ret);
    if (status != 0) {
      cerr << gai_strerror(status) << endl;
      exit(1);
    }

    socketfd = socket(host_info_ret->ai_family,
		      host_info_ret->ai_socktype,
		      host_info_ret->ai_protocol);
    
    if (socketfd == -1) {
      cerr << "Failed to open socket" << endl;
      exit(1);
    }
    
    status = connect(socketfd,
		     host_info_ret->ai_addr,
		     host_info_ret->ai_addrlen);
    if (status != 0) {
      cerr << "Failed to connect socket" << endl;
      exit(1);
    }
  }

  ~Endpoint()
  {
    int status = close(socketfd);
    if (status != 0) {
      cerr << "Failed to close socket: " << strerror(errno) << endl;
    }    
  }

  void send_request()
  {
    int bytes_sent = send(socketfd, reqstr.c_str(), reqstr.length(), 0);
  }

  void recv_response()
  {
    char buffer[1000];
    int bytes_recv = recv(socketfd, buffer, 1000, 0);
  }
};

void thr_fn(Endpoint *ep) {
  if (ep == nullptr)
    cerr << "null endpoint" << endl;

  for (int idx=0; idx < 1000; ++idx) {
    ep->send_request();
    ep->recv_response();
  }
  
  delete ep;
}

int main(int argc, char *argv[])
{
  /* Parse Arguments */
  struct arguments arguments;
  memset(&arguments, 0, sizeof (struct arguments));
  arguments.portstr = DEFAULT_PORT_STR;
  
  argp_parse(&argp, argc, argv, 0, 0, &arguments);
  
  cout << "Starting uvc..." << endl;

  vector<Endpoint*> endpoints;

  for (int idx=0; idx < 10; ++idx) {
    Endpoint *ep = new Endpoint(arguments.args[0], arguments.portstr, arguments.args[1]);
    endpoints.push_back(ep);
    cout << "Adding endpoint" << endl;
  }
  
  time_t ts;
  time(&ts);

  vector<thread*> threads;
  for (auto ep=endpoints.begin(); ep != endpoints.end(); ++ep) {
    thread *thr = new thread(thr_fn, *ep);
    threads.push_back(thr);
    cout << "added thread" << endl;
  }

  cout << "created threads" << endl;
  
  for (auto thr=threads.begin(); thr != threads.end(); ++thr) {
    (*thr)->join();
  }
  
  time_t te;
  time(&te);

  cout << "Took: " << (te - ts) << "s" << endl;

  return 0;
}
