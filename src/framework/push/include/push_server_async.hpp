//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef PUSH_SERVER_ASYNC_HPP
#define PUSH_SERVER_ASYNC_HPP

#include <set>
#include <deque>
#include <net.hpp>
 
class user
{
    public:
        virtual ~user() {}
        virtual void transfer(const buffer_t& buffer) = 0;
};

using user_t = std::shared_ptr<user>;

class groups
{
    public:
        void join(user_t user)
        {
            groups_.insert(user);
        }

        void leave(user_t user)
        {
            groups_.erase(user);
        }

        void transfer(const buffer_t& buffer)
        {
            for (auto& user: groups_)
                 user->transfer(buffer);
        }

    private:
        std::set<user_t> groups_;
};

class session : public user, public std::enable_shared_from_this<session>
{
    public:
        session(net::io_context& ioc, socket_t socket, groups& groups) :
        ioc_(ioc), socket_(std::move(socket)), groups_(groups)
        {
        }

        std::shared_ptr<session> shared_this()
        {
            return shared_from_this();
        }

        void run()
        {
            groups_.join(shared_this());
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
            {
                std::cout << "session closed" << std::endl;
                groups_.leave(shared_this());
                return;
            }

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
            {
                groups_.leave(shared_this());
                return;
            }

            carrier_.decode_message(buffer_);
            auto login = carrier_.message();
            std::cout << "message "  << login->message()  << std::endl;

            do_write();
        }

        void do_write()
        {
            net::async_write(socket_, net::buffer(buffer_),
            [self = shared_this()](error_code_t ec, size_t bytes_transferred)
            {
                self->on_write(ec, bytes_transferred);
            });
        }
    
        void on_write(error_code_t ec, size_t bytes_transferred)
        {
            if (ec)
            {
                groups_.leave(shared_this());
                return fail(ec, "write");
            }

            do_read_header();
        }

        void transfer(const buffer_t& buffer)
        {
            net::post(ioc_,
            [this, buffer]
            {
                bool write_in_progress = ! buffers_.empty();
                buffers_.push_back(buffer);
                if (! write_in_progress)
                    write_buffer();
            });
        }

        void write_buffer()
        {
            net::async_write(socket_, net::buffer(buffers_.front()),
            [self = shared_this(), this](error_code_t ec, size_t bytes_transferred)
            {
                if (! ec)
                {
                    buffers_.pop_front();
                    if (! buffers_.empty())
                        write_buffer();
                }
                else
                    groups_.leave(shared_this());
            });
        }

    private:
        net::io_context& ioc_;
        socket_t socket_;
        groups& groups_;
        buffer_t buffer_;
        carrier_t carrier_;
        std::deque<buffer_t> buffers_;
};

class listener : public std::enable_shared_from_this<listener>
{
    public:
        listener(net::io_context& ioc, endpoint_t endpoint) : ioc_(ioc), acceptor_(ioc)
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
                std::make_shared<session>(ioc_, std::move(socket), groups_)->run();

            do_accept();
        }

       groups& get()
       {
           return groups_;
       }

    private:
        net::io_context& ioc_;
        tcp::acceptor acceptor_;
        groups groups_;
};

#endif
