//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#include <chat_client_async.hpp>

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <host> <port>\n";
        return 1;
    }

    net::io_context ioc;
    chat_client c(ioc, argv[1], argv[2]);
    std::thread t([&ioc]{ ioc.run(); });

    buffer_t buffer_;
    carrier_t sender_;
    auto header = sender_.header();
    auto message = sender_.message();

    uint32_t seq = 0;
    std::string line;
    while (std::getline(std::cin, line))
    {
        header->set_seq(seq++);
        message->set_message(line);
        sender_.pack(buffer_);
        c.write(buffer_);
    }

    c.close();
    t.join();

    return 0;
}
