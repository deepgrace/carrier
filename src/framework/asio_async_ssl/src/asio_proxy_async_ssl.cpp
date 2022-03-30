//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#include <asio_proxy_async_ssl.hpp>

int main(int argc, char** argv)
{
    if (argc != 5)
    {
        std::cerr << "Usage:   " << argv[0] << " <host> <port> <host> <port>\n"
                  << "Example: " << argv[0] << " 127.0.0.1 8080 127.0.0.1 8081" << std::endl;
        return 1;
    }

    net::io_context ioc;
    address_t client({argv[1], argv[2]});
    address_t server({argv[3], argv[4]});
    std::make_shared<proxy>(ioc)->start(client, server);

    ioc.run();

    return 0;
}
