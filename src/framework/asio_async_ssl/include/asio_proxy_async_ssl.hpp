//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef ASIO_PROXY_ASYNC_SSL_HPP
#define ASIO_PROXY_ASYNC_SSL_HPP

#include <root_certificates.hpp>
#include <asio_async_ssl.hpp>

using address_t = std::pair<std::string, std::string>;

class proxy : public std::enable_shared_from_this<proxy>
{
    public:
        explicit proxy(net::io_context& ioc) : ctx(ssl::context::sslv23_client),
        client(net::make_strand(ioc), ctx), server(net::make_strand(ioc), ctx), resolver(net::make_strand(ioc))
        {
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
             do_resolve(client, lhs.first, lhs.second);
             do_resolve(server, rhs.first, rhs.second);
        }

        void close(socket_t& socket_)
        {
            error_code_t ec;
            socket_.lowest_layer().shutdown(net::socket_base::shutdown_both, ec);
            socket_.lowest_layer().close(ec);
        }

        void stop()
        {
            if (started_ < 2)
                return;

            started_ = 0;
            close(client);
            close(server);
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

            net::async_connect(socket_.lowest_layer(), results,
            std::bind(&proxy::on_connect, shared_this(), std::ref(socket_), std::placeholders::_1));
        }

        void on_connect(socket_t& socket_, error_code_t ec)
        {
            if (ec)
            {
                stop();
                return fail(ec, "connect");
            }

            socket_.async_handshake(ssl::stream_base::client,
            [self = shared_this()](error_code_t ec)
            {
                self->on_ssl_handshake(ec);
            });
        }

        void on_ssl_handshake(error_code_t ec)
        {
            if (ec)
                return fail(ec, "ssl_handshake");

            if (++started_ == 2) 
            {
                do_read_header(client, buffer_client, carrier_client);
                do_read_header(server, buffer_server, carrier_server);
            }
        }

        void do_read_header(socket_t& socket_, buffer_t& buffer_, carrier_t& carrier_)
        {
            buffer_.resize(header_size());
            net::async_read(socket_, net::buffer(buffer_),
            [&, self = shared_this()](error_code_t ec, size_t bytes_transferred)
            {
                self->on_read_header(socket_, buffer_, carrier_, ec, bytes_transferred);
            });
        }

        void on_read_header(socket_t& socket_, buffer_t& buffer_, carrier_t& carrier_, error_code_t ec, size_t bytes_transferred)
        {
            if (ec == net::error::eof)
                return stop();

            do_read_message(socket_, buffer_, carrier_);
        }

        void do_read_message(socket_t& socket_, buffer_t& buffer_, carrier_t& carrier_)
        {
            size_t size = carrier_.decode_header(buffer_);
            net::async_read(socket_, net::buffer(std::addressof(buffer_[header_size()]), size),
            [&, self = shared_this()](error_code_t ec, size_t bytes_transferred)
            {
                self->on_read_message(socket_, ec, bytes_transferred);
            });
        }

        void on_read_message(socket_t& socket_, error_code_t ec, size_t bytes_transferred)
        {
            if (ec == net::error::eof)
                return stop();

            if (! started())
                return;

            auto& buffer_ = &socket_ == &client ? buffer_client : buffer_server;
            do_write(&socket_ == &client ? server : client, buffer_, bytes_transferred);
        }

        void do_write(socket_t& socket_, buffer_t& buffer_, size_t bytes_transferred)
        {
            net::async_write(socket_, net::buffer(buffer_),
            [&, self = shared_this()](error_code_t ec, size_t bytes_transferred)
            {
                self->on_write(socket_, ec, bytes_transferred);
            });
        }

        void on_write(socket_t& socket_, error_code_t ec, size_t bytes_transferred)
        {
            if (ec)
            {
                stop();
                return fail(ec, "write");
            }

            if (&socket_ == &client)
                do_read_header(server, buffer_server, carrier_server);
            else
                do_read_header(client, buffer_client, carrier_client);
        }

    private:
        int started_ = 0;
        ssl::context ctx;
        socket_t client;
        socket_t server;
        tcp::resolver resolver;
        buffer_t buffer_client;
        buffer_t buffer_server;
        carrier_t carrier_client;
        carrier_t carrier_server;
};

#endif
