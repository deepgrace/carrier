# Carrier [![LICENSE](https://img.shields.io/github/license/deepgrace/carrier.svg)](https://github.com/deepgrace/carrier/blob/master/LICENSE_1_0.txt)

> **Modern C++ Network Server Framework**

Built with std::experimental::net, Boost.Asio, Boost.Beast and Google Protocol buffers.  
With client, server, proxy, gateway, push and file transfer services supported.

## Overview

## file transfer client async
```cpp
#include <file_transfer_client_async.hpp>

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <host> <port> [<file|dir>] ..." << std::endl;
        return 1;
    }

    net::io_context ioc;
    auto session_ = std::make_shared<session>(ioc, argv[1], argv[2]);

    std::thread t([&ioc]{ ioc.run(); });

    for (int i = 3; i != argc; ++i)
         session_->transfer(std::string(argv[i]));

    session_->transfer(fs::path("path/transfer/"));
    session_->transfer(std::string("[]<typename ...>(){}();\n"), "file/transfer.txt");

    t.join();

    return 0;
}
```
## file transfer client sync
```cpp
#include <file_transfer_client_sync.hpp>

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <host> <port> [<file|dir>] ..." << std::endl;
        return 1;
    }

    try
    {
        net::io_context ioc;
        auto session_ = std::make_shared<session>(ioc, argv[1], argv[2]);

        for (int i = 3; i != argc; ++i)
             session_->transfer(std::string(argv[i]));

        session_->transfer(fs::path("path/transfer/"));
        session_->transfer(std::string("[]<typename ...>(){}();\n"), "file/transfer.txt");
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
```
## file transfer server async
```cpp
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
```
