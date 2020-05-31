/// @file main.cpp
/// @author FPR (funny.pig.run __ ATMARK__ gmail.com)
///
/// @copyright Copyright (c) 2020
///
/// Part of the LaboHouse tool. Proprietary and confidential.
/// See the licenses directory for details.

#include <iostream>
#include <labo/server/Base.h>
#include <labo/debug/Log.h>
#include <llvm/Support/CommandLine.h>
#include <signal.h>

namespace labo::cl {
using namespace std;
using namespace llvm::cl;

/// List used commandline options here.
const vector<const OptionCategory*> related_categories{};
};

labo::Server* server{ nullptr };

void
terminate_server(int sig)
{
    using namespace labo;
    errs << "Signal received: " << strsignal(sig) << endl;
    if (server) {
        errs << "Terminating..." << endl;
        server->kill();
        delete server;
    } else {
        errs << "Server was not running." << endl;
    }
    logs << "Goodbye." << endl;
    ::exit(0);
};

int
main(int argc, char* argv[])
{
    // Command line stuff. It's magic.
    llvm::cl::HideUnrelatedOptions(labo::cl::related_categories);
    llvm::cl::ParseCommandLineOptions(argc, argv);

    signal(SIGQUIT, terminate_server);
    signal(SIGTERM, terminate_server);
    signal(SIGSTOP, terminate_server);
    signal(SIGINT, terminate_server);

    server = new labo::Server{ 12345 };
    server->start();
    return 0;
}