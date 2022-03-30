//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef CHAT_CLIENT_ASYNC_HPP
#define CHAT_CLIENT_ASYNC_HPP

#include <deque>
#include <net.hpp>

class chat_client
{
    public:
        chat_client(net::io_context& ioc, char* host, char* port) :
        socket_(ioc), ioc_(ioc), resolver_(ioc)
        {
            auto endpoints = resolver_.resolve(host, port);
            do_connect(endpoints);
        }

        void write(const buffer_t& buffer)
        {
            net::post(ioc_,
            [this, buffer]
            {
                bool write_in_progress = ! lines_.empty();
                lines_.push_back(buffer);
                if (! write_in_progress)
                    do_write();
            });
        }

        void close()
        {
            net::post(ioc_, [this]{ socket_.close(); });
        }

    private:
        void do_connect(const results_t& endpoints)
        {
            net::async_connect(socket_, endpoints,
            [this](error_code_t ec, endpoint_t)
            {
                on_connect(ec);
            });
        }   

        void on_connect(error_code_t ec)
        {
            if (! ec)
                do_read_header();
        }

        void do_read_header()
        {
            buffer_.resize(header_size());
            net::async_read(socket_, net::buffer(buffer_), 
            [this](error_code_t ec, size_t bytes_transferred)
            {
                on_read_header(ec, bytes_transferred);
            });
        }

        void on_read_header(error_code_t ec, size_t bytes_transferred)
        {
            if (! ec)
                do_read_message();
            else
                socket_.close();
        }

        void do_read_message()
        {
            size_t size = receiver_.decode_header(buffer_);
            net::async_read(socket_, net::buffer(std::addressof(buffer_[header_size()]), size),
            [this](error_code_t ec, size_t bytes_transferred)
            {
                on_read_message(ec, bytes_transferred);
            });
        }

        void on_read_message(error_code_t ec, size_t bytes_transferred)
        {
            if (! ec)
            {
                receiver_.decode_message(buffer_);
                std::cout << receiver_.message()->message() << std::endl;
                do_read_header();
            }
            else
                socket_.close();
        }

        void do_write()
        {
            net::async_write(socket_, net::buffer(lines_.front()),
            [this](error_code_t ec, size_t bytes_transferred)
            {
                if (! ec)
                {
                    lines_.pop_front();
                    if (! lines_.empty())
                        do_write();
                }
                else
                    socket_.close();
            });
        }

    private:
        buffer_t buffer_;
        carrier_t receiver_;
        socket_t socket_;
        net::io_context& ioc_;
        tcp::resolver resolver_;
        std::deque<buffer_t> lines_;
};

#endif
