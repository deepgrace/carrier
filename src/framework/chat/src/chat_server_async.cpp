//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#include <chat_server_async.hpp>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <port> ...\n";
        return 1;
    }

    net::io_context ioc;
    std::list<listener> servers;

    for (int i = 1; i < argc; ++i)
    {
        endpoint_t endpoint(tcp::v4(), std::atoi(argv[i]));
        servers.emplace_back(ioc, endpoint);
    }

    ioc.run();

    return 0;
}
