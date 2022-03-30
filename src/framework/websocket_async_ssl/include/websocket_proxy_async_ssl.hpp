//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef WEBSOCKET_PROXY_ASYNC_SSL_HPP
#define WEBSOCKET_PROXY_ASYNC_SSL_HPP

#include <root_certificates.hpp>
#include <websocket_async_ssl.hpp>

using address_t = std::pair<std::string, std::string>;

class proxy : public std::enable_shared_from_this<proxy>
{
    public:
        explicit proxy(net::io_context& ioc) : ctx(ssl::context::sslv23_client),
        client(net::make_strand(ioc), ctx), server(net::make_strand(ioc), ctx), resolver(net::make_strand(ioc))
        {
            setup_stream(client);
            setup_stream(server);
            load_root_certificates(ctx);
        }

        std::shared_ptr<proxy> shared_this()
        {
            return shared_from_this();
        }

        bool started()
        {
            return started_ == 2;
        }

        void start(const address_t& lhs, const address_t& rhs)
        {
             host_client = lhs.first;
             host_server = rhs.first;
             do_resolve(client, host_client, lhs.second);
             do_resolve(server, host_server, rhs.second);
        }

        void stop(socket_t& socket_)
        {
            if (started_ < 1)
                return;

            --started_;
            socket_.async_close(websocket::close_code::normal,
            [self = shared_this()](error_code_t ec)
            {
                self->on_close(ec);
            });
        }

        void on_close(error_code_t ec)
        {
            if (ec)
                return fail(ec, "close");
        }

        void do_resolve(socket_t& socket_, const std::string& host, const std::string& port)
        {
            resolver.async_resolve(host, port,
            [&, self = shared_this()](error_code_t ec, results_t results)
            {
                self->on_resolve(socket_, ec, results);
            });
        }

        void on_resolve(socket_t& socket_, error_code_t ec, results_t results)
        {
            if (ec)
                return fail(ec, "resolve");

            beast::get_lowest_layer(socket_).expires_after(std::chrono::seconds(30));

            beast::get_lowest_layer(socket_).async_connect(results,
            [self = shared_this(), &socket_](error_code_t ec, results_t::endpoint_type)
            {
                self->on_connect(socket_, ec);
            });
        }
    
        void on_connect(socket_t& socket_, error_code_t ec)
        {
            if (ec)
                return fail(ec, "connect");

            beast::get_lowest_layer(socket_).expires_after(std::chrono::seconds(30));

            socket_.next_layer().async_handshake(ssl::stream_base::client,
            [&, self = shared_this()](error_code_t ec)
            {
                self->on_ssl_handshake(socket_, ec);
            });
        }
    
        void on_ssl_handshake(socket_t& socket_, error_code_t ec)
        {
            if (ec)
                return fail(ec, "ssl_handshake");

            auto& host = &socket_ == &client ? host_client : host_server;

            beast::get_lowest_layer(socket_).expires_never();

            socket_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
            socket_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-proxy-async-ssl");
            }));

            socket_.async_handshake(host, "/",
            [&, self = shared_this()](error_code_t ec)
            {
                self->on_handshake(socket_, ec);
            });
        }

        void on_handshake(socket_t& socket_, error_code_t ec)
        {
            if (ec)
            {
                stop(socket_);
                return fail(ec, "handshake");
            }

            if (++started_ == 2) 
            {
                do_read(client, buffer_client);
                do_read(server, buffer_server);
            }
        }

        void do_read(socket_t& socket_, buffer_t& buffer_)
        {
            socket_.async_read(buffer_,
            [&, self = shared_this()](error_code_t ec, size_t bytes_transferred)
            {
                self->on_read(socket_, ec, bytes_transferred);
            });
        }

        void on_read(socket_t& socket_, error_code_t ec, size_t bytes_transferred)
        {
            if (ec == websocket::error::closed || ec == net::error::eof)
                return stop(socket_);

            if (! started())
                return;

            auto& buffer_ = &socket_ == &client ? buffer_client : buffer_server;
            do_write(&socket_ == &client ? server : client, buffer_, bytes_transferred);
        }

        void do_write(socket_t& socket_, buffer_t& buffer_, size_t bytes_transferred)
        {
            socket_.async_write(buffer_.data(),
            [&, self = shared_this()](error_code_t ec, size_t bytes_transferred)
            {
                self->on_write(socket_, ec, bytes_transferred);
            });
        }

        void on_write(socket_t& socket_, error_code_t ec, size_t bytes_transferred)
        {
            if (ec)
            {
                stop(socket_);
                return fail(ec, "write");
            }

            auto& socket = &socket_ == &client ? server : client;
            auto& buffer = &socket_ == &client ? buffer_server : buffer_client;
            buffer.consume(buffer.size());
            do_read(socket, buffer);
        }

    private:
        int started_ = 0;
        ssl::context ctx;
        socket_t client;
        socket_t server;
        tcp::resolver resolver;
        buffer_t buffer_client;
        buffer_t buffer_server;
        std::string host_client;
        std::string host_server;
};

#endif
