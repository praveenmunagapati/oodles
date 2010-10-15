// oodles scheduler
#include "sched/Context.hpp"

// oodles utility
#include "utility/file-ops.hpp"

// STL
#include <string>
#include <fstream>
#include <iostream>

// libc
#include <getopt.h>

// STL
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ostream;
using std::ofstream;
using std::exception;

// oodles utility
using oodles::read_file_data;
// oodles scheduler
using oodles::sched::Context;

static void print_usage(const char *program)
{
    cerr << program
         << ":\n"
         << "\n-h\t--help"
         << "\n-s\t--service <ip:port>"
         << "\n-f\t--seed-file <seed input file>"
         << "\n-d\t--dot-file <dot output file>\n";
}

int main(int argc, char *argv[])
{
    int ch = -1;
    string seed_file, dot_file;
    string listen_on("127.0.0.1:8888");
    const char *short_options = "hs:f:d:";
    const struct option long_options[5] = {
        {"help", no_argument, NULL, short_options[0]},
        {"service", required_argument, NULL, short_options[1]},
        {"seed-file", required_argument, NULL, short_options[3]},
        {"dot-file", required_argument, NULL, short_options[5]},
        {NULL, 0, NULL, 0}
    };

    while ((ch = getopt_long(argc, argv,
                             short_options,
                             long_options, NULL)) != -1)
    {
        switch (ch) {
            case 'd':
                dot_file = optarg;
                break;
            case 'f':
                seed_file = optarg;
                break;
            case 's':
                listen_on = optarg;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (seed_file.empty()) {
        print_usage(argv[0]);
        return 1;
    }

    int rc = 0;
    
    try {
        string s;
        ostream *dot_stream = NULL;
        size_t n = read_file_data(seed_file, s);

        if (n != s.size())
            return 1;

        if (!dot_file.empty()) {
            try {
                dot_stream = new ofstream(dot_file.c_str());
            } catch (const exception &e) {
                cerr << "Failed to open '" << dot_file << "' for writing.\n";
                return 1;
            }
        }
        
        Context context;
        string::size_type b = 0, e = s.find_first_of('\n', b), t = string::npos;

        /* Allow Crawlers to sit connected */
        context.start_server(listen_on);

        while (e != t) {
            context.seed_scheduler(s.substr(b, e - b));
            
            b = e + 1;
            e = s.find_first_of('\n', b);
        }

        s.clear(); // We no longer need this data
        context.start_crawling(dot_stream, 5); // 5s interval between runs

        delete dot_stream;
    } catch (const exception &e) {
        cerr << e.what() << endl;
        rc = 1;
    }

    return rc;
}
