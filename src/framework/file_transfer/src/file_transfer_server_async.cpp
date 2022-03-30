//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#include <file_transfer_server_async.hpp>

int main(int argc, char* argv[])
{
    std::string host("0.0.0.0");
    unsigned short port = 2020;

    if (argc == 4)
    {
        host = argv[1];
        port = static_cast<unsigned short>(std::atoi(argv[2]));
    }
    else if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " [<host> <port>] <path>\n";
        return 1;
    }

    auto const host_ = net::ip::make_address(host);
    auto const threads = static_cast<int>(std::thread::hardware_concurrency());

    net::io_context ioc{threads};
    std::make_shared<listener>(ioc, endpoint_t{host_, port}, argv[argc - 1])->run();

    std::vector<std::thread> v;
    v.reserve(threads - 1);

    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back([&ioc]{ ioc.run(); });
    ioc.run();

    return 0;
}
