//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef NET_SERVER_ASYNC_HPP
#define NET_SERVER_ASYNC_HPP

#include <net.hpp>
 
class session : public std::enable_shared_from_this<session>
{
    public:
        session(socket_t socket) :
        socket_(std::move(socket)), strand_(socket_.get_executor())
        {
        }

        std::shared_ptr<session> shared_this()
        {
            return shared_from_this();
        }

        void run()
        {
            do_read_header();
        }
    
        void do_read_header()
        {
            buffer_.resize(header_size());
            net::async_read(socket_, net::buffer(buffer_), net::bind_executor(strand_,
            [self = shared_this()](error_code_t ec, size_t bytes_transferred)
            {
                self->on_read_header(ec, bytes_transferred);
            }));
        }

        void on_read_header(error_code_t ec, size_t bytes_transferred)
        {
            if (ec == net::error::eof)
                return;

            do_read_message();
        }

        void do_read_message()
        {
            size_t size = carrier_.decode_header(buffer_);
            net::async_read(socket_, net::buffer(std::addressof(buffer_[header_size()]), size), net::bind_executor(strand_,
            [self = shared_this()](error_code_t ec, size_t bytes_transferred)
            {
                self->on_read_message(ec, bytes_transferred);
            }));
        }
        
        void on_read_message(error_code_t ec, size_t bytes_transferred)
        {
            if (ec == net::error::eof)
                return;

            do_write();
        }

        void do_write()
        {
            net::async_write(socket_, net::buffer(buffer_), net::bind_executor(strand_,
            [self = shared_this()](error_code_t ec, size_t bytes_transferred)
            {
                self->on_write(ec, bytes_transferred);
            }));
        }
    
        void on_write(error_code_t ec, size_t bytes_transferred)
        {
            if (ec)
                return fail(ec, "write");

            do_read_header();
        }

    private:
        socket_t socket_;
        strand_t strand_;
        buffer_t buffer_;
        carrier_t carrier_;
};

class listener : public std::enable_shared_from_this<listener>
{
    public:
        listener(net::io_context& ioc, endpoint_t endpoint) : acceptor_(ioc)
        {
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
            acceptor_.async_accept(
            [self = shared_from_this()](error_code_t ec, socket_t socket)
            {
                self->on_accept(ec, std::move(socket));
            });
        }

        void on_accept(error_code_t ec, socket_t socket)
        {
            if (ec)
                fail(ec, "accept");
            else
                std::make_shared<session>(std::move(socket))->run();

            do_accept();
        }

    private:
        tcp::acceptor acceptor_;
};

#endif
