//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef PUSH_CLIENT_ASYNC_HPP
#define PUSH_CLIENT_ASYNC_HPP

#include <net.hpp>

class session : public std::enable_shared_from_this<session>
{
    public:
        explicit session(net::io_context& ioc, const std::string& username, const std::string& password) :
        resolver_(ioc), socket_(ioc), username_(username), password_(password)
        {
        }

        std::shared_ptr<session> shared_this()
        {
            return shared_from_this();
        }

        void run(const std::string& host, const std::string& port)
        {
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

            if (login())
                do_read_header();
        }

        bool login()
        {
            auto message = carrier_.message();
            message->set_message("login message");

            carrier_.pack(buffer_);
            net::write(socket_, net::buffer(buffer_));

            buffer_.resize(header_size());
            net::read(socket_, net::buffer(buffer_));

            size_t size = carrier_.decode_header(buffer_);
            net::read(socket_, net::buffer(std::addressof(buffer_[header_size()]), size));

            carrier_.decode_message(buffer_);
            std::cout << carrier_.message()->message() << std::endl;
            return true;
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
            do_read_header();
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
        std::string username_;
        std::string password_;
        buffer_t buffer_;
        carrier_t carrier_;
};

#endif
