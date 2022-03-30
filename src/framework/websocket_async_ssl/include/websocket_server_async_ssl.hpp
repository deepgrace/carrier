//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef WEBSOCKET_SERVER_ASYNC_SSL_HPP
#define WEBSOCKET_SERVER_ASYNC_SSL_HPP

#include <websocket_async_ssl.hpp>
#include <server_certificate.hpp>

class session : public std::enable_shared_from_this<session>
{
    public:
        session(tcp::socket socket, ssl::context& ctx) :
        ws_(std::move(socket), ctx)
        {
            setup_stream(ws_);
        }

        std::shared_ptr<session> shared_this()
        {
            return shared_from_this();
        }

        void run()
        {
            beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

            ws_.next_layer().async_handshake(ssl::stream_base::server,
            [self = shared_this()](error_code_t ec)
            {
                self->on_ssl_handshake(ec);
            });
        }
    
        void on_ssl_handshake(error_code_t ec)
        {
            if (ec)
                return fail(ec, "ssl_handshake");

            beast::get_lowest_layer(ws_).expires_never();

            ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

            ws_.set_option(websocket::stream_base::decorator(
            [](websocket::response_type& res)
            {
                res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async-ssl");
            }));

            ws_.async_accept(
            [self = shared_this()](error_code_t ec)
            {
                self->on_accept(ec);
            });
        }
    
        void on_accept(error_code_t ec)
        {
            if (ec)
                return fail(ec, "accept");

            do_read();
        }
    
        void do_read()
        {
            ws_.async_read(buffer_, 
            [self = shared_this()](error_code_t ec, size_t bytes_transferred)
            {
                self->on_read(ec, bytes_transferred);
            });
        }

        void on_read(error_code_t ec, size_t bytes_transferred)
        {
            if (ec == websocket::error::closed || ec == net::error::eof)
                return;

            do_write();
        }

        void do_write()
        {
            ws_.async_write(buffer_.data(),
            [self = shared_this()](error_code_t ec, size_t bytes_transferred)
            {
                self->on_write(ec, bytes_transferred);
            });
        }
    
        void on_write(error_code_t ec, size_t bytes_transferred)
        {
            if (ec)
                return fail(ec, "write");

            buffer_.consume(buffer_.size());
            do_read();
        }

    private:
        socket_t ws_;
        buffer_t buffer_;
};

class listener : public std::enable_shared_from_this<listener>
{
    public:
        listener(net::io_context& ioc, endpoint_t endpoint) :
        ioc_(ioc), acceptor_(ioc), ctx_server_(ssl::context::sslv23)
        {
            load_server_certificate(ctx_server_);

            error_code_t ec;
            acceptor_.open(endpoint.protocol(), ec);
            if (ec)
            {
                fail(ec, "open");
                return;
            }

            acceptor_.set_option(net::socket_base::reuse_address(true), ec);
            if (ec)
            {
                fail(ec, "set_option");
                return;
            }

            acceptor_.bind(endpoint, ec);
            if (ec)
            {
                fail(ec, "bind");
                return;
            }

            acceptor_.listen(net::socket_base::max_listen_connections, ec);
            if (ec)
            {
                fail(ec, "listen");
                return;
            }
        }

        void run()
        {
            if (! acceptor_.is_open())
                return;

            do_accept();
        }
    
        void do_accept()
        {
            acceptor_.async_accept(net::make_strand(ioc_),
            [self = shared_from_this()](error_code_t ec, tcp::socket socket)
            {
                self->on_accept(ec, std::move(socket));
            });
        }

        void on_accept(error_code_t ec, tcp::socket socket)
        {
            if (ec)
                fail(ec, "accept");
            else
                std::make_shared<session>(std::move(socket), ctx_server_)->run();

            do_accept();
        }

    private:
        net::io_context& ioc_;
        tcp::acceptor acceptor_;
        ssl::context ctx_server_;

};

#endif
