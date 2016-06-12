#include <iostream>
using std::cerr;
using std::cout;
using std::endl;
using std::exception;

#include <argp.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <chrono>

#include <memory>
using std::make_shared;

#include <poll.h>

#include "UVBSocketSpawner.hpp"
#include "Scheduler.hpp"
#include "UVBSocket.hpp"
#include "uvc.hpp"

char const *DEFAULT_PORT_STR = "80";
const int ARGC_COUNT = 2;
const char *uvc_program_version		= "uvc v0.1";
const char *uvc_program_bug_address	= "<wdignazio@gmail.com>";
const char doc[] = "Ultimate Victory Battle Client";

static char args_doc[] = "URL NAME";

static struct argp_option options[] =
    {
        { "port", 'p', "PORT", 0, "Set port for UVB endpoint" },
        { "threads", 't', "THREADS", 0, "Worker thread count, defaults to physical CPU's" },
	{ "sockets", 's', "SOCKETS", 0, "Number of sockets to open, defaults to 10" },
        { "spawners", 'w', "SPAWNERS", 0, "Number of spawners that open the socket count" },
        { 0 }
    };

struct arguments
{
    char *args[2];
    char const *portstr;
    int nthreads;
    int nsockets;
    int nspawners;
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = (struct arguments*)state->input;
  
    switch (key) {
    case 'p':
        arguments->portstr = arg;
        break;

    case 't':
        arguments->nthreads = std::stoi(arg);
        break;

    case 's':
        arguments->nsockets = std::stoi(arg);
        break;

    case 'w':
        arguments->nspawners = std::stoi(arg);
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
    arguments.nthreads = std::thread::hardware_concurrency();
    arguments.nsockets = 10;
    arguments.nspawners = 4;
  
    argp_parse(&argp, argc, argv, 0, 0, &arguments);
  
    cout << "Starting uvc..." << endl;
    cout << "host: " << arguments.args[0] << endl;
    cout << "port: " << arguments.portstr << endl;
    cout << "name: " << arguments.args[1] << endl;

    const string payload = fill_req_template(arguments.args[0], arguments.args[1]);

    cout << "Payload:" << endl;
    cout << payload << endl;

    try {
        string host{arguments.args[0]};
        string portstr{arguments.portstr};
        int nsockets{arguments.nsockets};
        
        UVBSocketSpawner spawner(arguments.nspawners, host, portstr, payload);
        
        vector<shared_ptr<UVBSocket>> sockets{spawner.spawn(nsockets)};
        vector<Scheduler*> schedulers{};
        
        unsigned int sched_batch_count = sockets.size() / 1000;
        if (sched_batch_count == 0)
            sched_batch_count = 1;

        const auto first = sockets.begin();
        for (unsigned int idx=0; idx < sched_batch_count; ++idx) {
            if ((idx + 1) * 1000 > sockets.size()) {
                vector<shared_ptr<UVBSocket>> final(first, sockets.end());

                int nthreads = arguments.nthreads / sched_batch_count;
                if (nthreads == 0)
                    nthreads = 1;
                
                Scheduler *sched = new Scheduler(final, nthreads);
                sched->start();
                
                schedulers.push_back(sched);

                cout << "Spawned last one" << endl;
                break;
            }

            vector<shared_ptr<UVBSocket>> subsockets(first, first+1000);
            Scheduler *sched = new Scheduler(subsockets, arguments.nthreads);
            sched->start();

            schedulers.push_back(sched);
            cout << "here" << endl;
        }
       
        for (;;) {
            sleep(10);
        }
    
    } catch (const exception &exc) {
        cerr << "ERROR: "  << exc.what() << endl;
    }

    return 0;
}
