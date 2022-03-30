//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef CHAT_SERVER_ASYNC_HPP
#define CHAT_SERVER_ASYNC_HPP

#include <set>
#include <list>
#include <deque>
#include <ctime>
#include <string>
#include <net.hpp>

std::string timestamp()
{
    char date[80];
    timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    tm* timeinfo = localtime(static_cast<time_t*>(&tp.tv_sec));
    strftime(date, 80, "%Y-%m-%d-%H:%M:%S", timeinfo);
    return std::string(date);
}

class char_user
{
    public:
        virtual ~char_user() {}
        virtual void deliver(const buffer_t& buffer) = 0;
};

using user_t = std::shared_ptr<char_user>;

class chat_room
{
    public:
        void join(user_t user)
        {
            group_.insert(user);
        }

        void leave(user_t user)
        {
            group_.erase(user);
        }

        void deliver(const buffer_t& buffer)
        {
            for (auto user: group_)
                 user->deliver(buffer);
        }

    private:
        std::set<user_t> group_;
};

class chat_session : public char_user, public std::enable_shared_from_this<chat_session>
{
    public:
        chat_session(socket_t socket, chat_room& room) :
        socket_(std::move(socket)), room_(room)
        {
        }

        std::shared_ptr<chat_session> shared_this()
        {
            return shared_from_this();
        }
    
        void start()
        {
            room_.join(shared_this());
            do_read_header();
        }

        void deliver(const buffer_t& buffer)
        {
            bool write_in_progress = !lines_.empty();
            lines_.push_back(buffer);
            if (! write_in_progress)
                do_write();
        }

    private:
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
            if (! ec)
                do_read_message();
            else
                room_.leave(shared_this());
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
            if (! ec)
            {
                prepend_timestamp();
                room_.deliver(buffer_);
                do_read_header();
            }
            else
                room_.leave(shared_this());
        }

        void do_write()
        {
            net::async_write(socket_, net::buffer(lines_.front()),
            [this, self = shared_this()](error_code_t ec, size_t bytes_transferred)
            {
                if (! ec)
                {
                    lines_.pop_front();
                    if (! lines_.empty())
                        do_write();
                }
                else
                    room_.leave(shared_this());
            });
        }

        void prepend_timestamp()
        {
            carrier_.decode_message(buffer_);
            auto data = carrier_.message();
            std::string time("<");
            time.append(timestamp());
            time.append(1, '>');
            time.append(1, ' ');
            time.append(data->message());
            data->set_message(time);
            carrier_.pack(buffer_);
        }

        socket_t socket_;
        chat_room& room_;
        buffer_t buffer_;
        carrier_t carrier_;
        std::deque<buffer_t> lines_;
};

class listener
{
    public:
        listener(net::io_context& io_context, const endpoint_t& endpoint) : acceptor_(io_context, endpoint)
        {
            do_accept();
        }

    private:
        void do_accept()
        {
            acceptor_.async_accept(
            [this](error_code_t ec, socket_t socket)
            {
                if (!ec)
                    std::make_shared<chat_session>(std::move(socket), room_)->start();
                do_accept();
            });
        }

        tcp::acceptor acceptor_;
        chat_room room_;
};

#endif
