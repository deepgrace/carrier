//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#include <push_client_async.hpp>

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage:   " << argv[0] << " <host> <port>\n"
                  << "Example: " << argv[0] << " 127.0.0.1 8080\n";
        return 1;
    }

    auto const host = argv[1];
    auto const port = argv[2];

    net::io_context ioc;
    std::string username = "root";
    std::string password = "****";
    std::make_shared<session>(ioc, username, password)->run(host, port);
    ioc.run();

    return 0;
}
