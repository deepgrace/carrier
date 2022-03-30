//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef NET_CLIENT_ASYNC_HPP
#define NET_CLIENT_ASYNC_HPP

#include <net.hpp>

class session : public std::enable_shared_from_this<session>
{
    public:
        explicit session(net::io_context& ioc, int srv): 
        resolver_(ioc), socket_(ioc) , srv_(srv)
        {
        }

        std::shared_ptr<session> shared_this()
        {
            return shared_from_this();
        }

        void run(const std::string& host, const std::string& port, const std::string& text)
        {
            carrier_.message()->set_message(text);
            resolver_.async_resolve(host, port,
            [self = shared_this()](error_code_t ec, results_t results)
            {
                self->on_resolve(ec, results);
            });
        }

        void on_resolve(error_code_t ec, results_t results)
        {
            if (ec)
                return fail(ec, "resolve");

            net::async_connect(socket_, results,
            std::bind(&session::on_connect, shared_this(), std::placeholders::_1));
        }

        void on_connect(error_code_t ec)
        {
            if (ec)
               return fail(ec, "connect");

            do_write();
        }

        void do_write()
        {
            auto header = carrier_.header();
            header->set_seq(8080);
            header->set_service(srv_);
            carrier_.pack(buffer_);

            net::async_write(socket_, net::buffer(buffer_),
            [self = shared_this()](error_code_t ec, size_t bytes_transferred)
            {
                self->on_write(ec, bytes_transferred);
            });
        }

        void on_write(error_code_t ec, size_t bytes_transferred)
        {
            if (ec)
                return fail(ec, "write");

            do_read_header();
        }

        void do_read_header()
        {
            buffer_.resize(header_size());
            net::async_read(socket_, net::buffer(buffer_),
            [self = shared_this()](error_code_t ec, size_t bytes_transferred)
            {
                self->on_read_header(ec, bytes_transferred);
            });
        }

        void on_read_header(error_code_t ec, size_t bytes_transferred)
        {
            if (ec == net::error::eof)
                return fail(ec, "read");

            do_read_message();
        }

        void do_read_message()
        {
            size_t size = carrier_.decode_header(buffer_);
            net::async_read(socket_, net::buffer(std::addressof(buffer_[header_size()]), size),
            [self = shared_this()](error_code_t ec, size_t bytes_transferred)
            {
                self->on_read_message(ec, bytes_transferred);
            });
        }
        
        void on_read_message(error_code_t ec, size_t bytes_transferred)
        {
            if (ec == net::error::eof)
                return fail(ec, "read");

            carrier_.decode_message(buffer_);
            std::cout << carrier_.message()->message() << std::endl;
            do_close();
        }

        void do_close()
        {
            error_code_t ec;
            socket_.shutdown(net::socket_base::shutdown_both, ec);
            socket_.close(ec);
        }

    private:
        tcp::resolver resolver_;
        socket_t socket_;
        buffer_t buffer_;
        carrier_t carrier_;
        int srv_;
};

#endif
