/// @file http.cpp
/// @author FPR (funny.pig.run __ ATMARK__ gmail.com)
///
/// @copyright Copyright (c) 2020
///
/// Part of the LaboHouse tool. Proprietary and confidential.
/// See the licenses directory for details.
#include <labo/debug/Log.h>
#include <labo/server/http/Request.h>
#include <limits>
#include <regex>

namespace labo::http {
void
Request::deserialize(istream& is)
{
    auto check_is = [&]() {
        if (is.bad()) {
            errs << "Parsing error." << endl;
            failure();
        }
    };

    logs << "Request: Parsing..." << endl;
    string raw_method;
    is >> raw_method;
    check_is();
    if (raw_method == "GET") {
        method = Method::GET;
    } else if (raw_method == "POST") {
        method = Method::POST;
    } else {
        errs << "Unexpected Request: " << raw_method << endl;
        failure();
    }
    logs << "Method: " << raw_method << endl;

    is >> path;
    check_is();
    {
        smatch matches;
        static const regex uri_pattern{ "^([^\?]+)(.*)$" };
        if (regex_match(path, matches, uri_pattern)) {
            for (auto match : matches) {
                logs << "Match: " << match << endl;
            }
        } else {
            logs << "no matches" << endl;
        }
        logs << path << endl;
    }

    string protocol;
    is >> protocol;
    check_is();
    logs << "Protocol Version: " << protocol << endl;

    for (string line; getline(is, line);) {
        if (line.size() == 0 || line == "\r") {
            break;
        }
        const regex header_pattern{
            "^([^:]*): (.*)(\r)*"
        }; // HOLY SHIT FUCK YOU CARRIGE RETURN. FUCK YOU FUCK YOU FFUCK YOU.
        smatch matches;
        if (regex_match(line, matches, header_pattern) && matches.size() > 2) {
            headers.insert({ matches[1], matches[2] });
            logs << "[ " << matches[1] << ", " << matches[2] << " ]" << endl;
        } else {
            errs << "Failed to match header: " << line << endl;
            errs << matches.size() << endl;
        }
    }

    logs << "Request: Done parsing!" << endl;
}
};