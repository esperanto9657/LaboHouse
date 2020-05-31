/// @file Server.cpp
/// @author FPR (funny.pig.run __ ATMARK__ gmail.com)
///
/// @copyright Copyright (c) 2020
///
/// Part of the LaboHouse tool. Proprietary and confidential.
/// See the licenses directory for details.
#include <algorithm>
#include <labo/server/Base.h>
#include <labo/debug/Log.h>
#include <netinet/in.h>
#include <sstream>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace labo {
void
Server::action(int socket_fd)
{
    logs << "Start hoge: " << socket_fd << endl;
    char buffer[256];
    ::bzero(buffer, 256);
    {
        auto size{ ::read(socket_fd, buffer, 255) };
        if (size < 0) {
            errs << "Input error" << endl;
        }
        logs << "Received: " << buffer << endl;
    }
    {
        stringstream ss;
        ss << "echo: " << buffer;
        string reply{ ss.str() };
        auto size{ ::write(socket_fd, reply.c_str(), reply.size()) };
        if (size < 0) {
            errs << "Output error" << endl;
        }
    }

    logs << "Finish hoge" << endl;
}

void
Server::run_action(int socket_fd)
{
    action(socket_fd);
    ::close(socket_fd);
}

Server::Server(int port)
  : port{ port }
  , exit{ false } {};

void
Server::start()
{
    // Try to open socket.
    auto socket_fd{ ::socket(AF_INET, SOCK_STREAM, 0) };
    if (socket_fd < 0) {
        errs << "Failed to create socket." << endl;
        failure();
    }

    // Close your eyes, some nasty legacy C code
    // We want to pass this struct with address information.
    sockaddr_in server_address;
    ::bzero((char*)&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = ::htons(port);

    // Bind our socket to an address
    if (::bind(socket_fd, (sockaddr*)&server_address, sizeof(server_address)) <
        0) {
        errs << "Failed to bind to port: " << port << endl;
        failure();
    }

    // Start listening. Standby for new connections.
    ::listen(socket_fd, 8);

    logs << "Server started at port: " << port << endl;

    sockaddr_in client_address;
    uint client_address_length;
    while (!exit) {
        auto personal_socket_fd{ ::accept(
          socket_fd, (sockaddr*)&client_address, &client_address_length) };
        if (personal_socket_fd < 0) {
            errs << "Failed to accept connection." << endl;
            continue;
        }

        logs << "Accepted connection from: " << client_address.sin_addr.s_addr
             << ":" << client_address.sin_port << endl;
        threads.insert(new thread{ Server::run_action, personal_socket_fd });
    }
    ::close(socket_fd);
}

void
Server::kill()
{
    logs << "Server kill request received." << endl;
    exit = true;
}

Server::~Server()
{
    logs << "Waiting for all threads to finish..." << endl;
    for_each(threads.begin(), threads.end(), [](auto thread_ptr) {
        thread_ptr->join();
    });
    logs << "Server terminated." << endl;
}
};