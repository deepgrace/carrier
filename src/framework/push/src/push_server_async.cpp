//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#include <push_server_async.hpp>

std::string timestamp()
{
    char date[80];
    timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    tm* timeinfo = localtime(static_cast<time_t*>(&tp.tv_sec));
    strftime(date, 80, "%Y-%m-%d-%H:%M:%S", timeinfo);
    return std::string(date);
}


std::string prepend_timestamp(const std::string& message)
{
    std::string time("<");
    time.append(timestamp());
    time.append(1, '>');
    time.append(1, ' ');
    time.append(message);
    return time;
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage:   " << argv[0] << " <host> <port>\n"
                  << "Example: " << argv[0] << " 0.0.0.0 8080\n";
        return 1;
    }

    auto const host = net::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));

    net::io_context ioc;
    auto server = std::make_shared<listener>(ioc, endpoint_t{host, port});
    server->run();

    std::thread t([&ioc]{ ioc.run(); });

    buffer_t buffer_;
    carrier_t carrier_;
    auto header = carrier_.header();
    auto message = carrier_.message();

    std::string line;
    auto& groups = server->get();

    while (std::getline(std::cin, line))
    {
        message->set_message(prepend_timestamp(line));
        carrier_.pack(buffer_);
        groups.transfer(buffer_);
    }

    t.join();

    return 0;
}
