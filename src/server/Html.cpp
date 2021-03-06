/// @file Html.cpp
/// @author FPR (funny.pig.run __ ATMARK__ gmail.com)
///
/// @copyright Copyright (c) 2020
///
/// Part of the SWORD tool. Proprietary and confidential.
/// See the licenses directory for details.

#include "labo/server/http/Html.h"
#include "labo/LaboHouse.h"
#include <algorithm>
#include <future>
#include <labo/debug/Log.h>
#include <labo/server/Html.h>
#include <labo/server/http/Request.h>
#include <labo/server/http/Response.h>
#include <labo/server/http/ResponseHelper.h>
#include <labo/util/fdstreambuf.h>
#include <netinet/in.h>
#include <sstream>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

namespace labo::server {

Html::Html(LaboHouse& lh)
  : lh{ lh }
{}

vector<future<void>> threads;

void
Html::start(const int port)
{
    // Try to open socket.
    sfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) {
        errs << "[Html] Failed to create socket." << endl;
        failure();
    }

    // Close your eyes, some nasty legacy C code
    // We want to pass this struct with address information.
    sockaddr_in server_address;
    ::bzero((char*)&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    {
        // Override the socket for faster testing.
        int option{ 1 };
        setsockopt(
          sfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&option, sizeof(option));
        setsockopt(
          sfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&option, sizeof(option));
    }

    // Bind our socket to an address
    if (::bind(sfd, (sockaddr*)&server_address, sizeof(server_address)) < 0) {
        errs << "[Html] Failed to bind to port: " << port << endl;
        failure();
    }

    // Start listening. Standby for new connections.
    ::listen(sfd, 32);

    logs << "[Html] Started at port: " << port << endl;

    sockaddr_in client_address;
    uint client_address_length;
    while (true) {
        auto fd{ ::accept(
          sfd, (sockaddr*)&client_address, &client_address_length) };
        if (fd < 0) {
            errs << "[Html] Failed to accept connection." << endl;
            continue;
        }

        logs << "[Html] Accepted connection from: "
             << client_address.sin_addr.s_addr << ":" << client_address.sin_port
             << endl;

        threads.emplace_back(async(launch::async, [fd, &lh = lh] {
            using namespace labo;
            using namespace http;
            socket::fdstreambuf stream{ fd };
            istream in{ &stream };
            ostream out{ &stream };

            Request req;
            in >> req;
            if (!req.valid) {
                logs << "[Html] Invalid Request." << endl;
                return;
            } else if (req.method() == Request::Method::GET &&
                       req.path() == "/") {
                map<string, string> headers{
                    { "Content-Security-Policy",
                      "connect-src 'self' "
                      "ws://funnypigrun.dns-cloud.net:12273" },
                    { "Known", "no" },
                };
                auto ocookie{ req.header_value("Cookie") };
                if (ocookie) {
                    auto ouser{ lh.users.by_cookie(ocookie.get()) };
                    if (ouser) {
                        headers.at("Known") = "yes";
                        logs << "[Html] User is known. ID: " << ouser.get().id
                             << "." << endl;
                    }
                }

                /// reply with home page
                out << Response{ Response::Status::OK,
                                 http::Html{ "res/home.html" },
                                 headers }
                    << endl
                    << endl;
                logs << "[Html] Homepage access." << endl;
            } else if (req.method() == http::Request::Method::POST &&
                       req.path() == "/register") {
                auto oname{ req.header_value("name") };
                auto oid{ req.header_value("id") };

                if (!oname) {
                    out << bad_request("Missing name.");
                    logs << "[Html] Bad request: Missing name." << endl;
                    return;
                }
                if (!oid) {
                    out << bad_request("Missing id.");
                    logs << "[Html] Bad request: Missing id." << endl;
                    return;
                }

                if (lh.users.by_id(oid.get())) {
                    out << unauthorized("ID already taken: " + oid.get());
                    logs << "[Html] Bad request: ID already taken: "
                         << oid.get() << "." << endl;
                    return;
                }

                auto& u{ lh.users.add(oid.get()) };
                lh.chats.get("All")->chat(u, "has joined! Say hello =)");
                u.name = oname.get();

                out << Response{ Response::Status::OK,
                                 { { "Set-Cookie",
                                     u.cookie + "; SameSite=Strict" } } }
                    << endl
                    << endl;
            } else if (req.method() == http::Request::Method::GET &&
                       req.path() == "/favicon.ico") {
                out << Response{ Response::Status::OK,
                                 http::Image{ "../res/favicon.ico" } }
                    << endl
                    << endl;
            } else if (req.method() == http::Request::Method::GET &&
                       req.path() == "/human.png") {
                out << Response{ Response::Status::OK,
                                 http::Image{ "../res/human.png" } }
                    << endl
                    << endl;
            }else {
                out << Response{ Response::Status::NOT_FOUND,
                                 http::Html{ "res/not_found.html" } };
                errs << "[Html] Unknown request." << endl;
            }

            ::close(fd);
        }));

        threads.erase(remove_if(threads.begin(),
                                threads.end(),
                                [](future<void>& f) {
                                    if (f.wait_for(0ms) ==
                                        future_status::ready) {
                                        f.get();
                                        return true;
                                    }
                                    return false;
                                }),
                      threads.end());
    }
}

Html::~Html()
{
    logs << "[Html] Server stopped." << endl;
    ::close(sfd);
}
};