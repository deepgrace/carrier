//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef NET_GATEWAY_ASYNC_HPP
#define NET_GATEWAY_ASYNC_HPP

#include <net.hpp>
#include <load_config.hpp>

template <typename T>
class request : public std::enable_shared_from_this<request<T>>
{
    public:
        request(T& g, socket_t socket) : gw(g),
        socket_(std::move(socket)), strand_(socket_.get_executor())
        {
        }
        
        socket_t& get()
        {
            return socket_;
        }
    
        std::shared_ptr<request<T>> shared_this()
        {
            return this->shared_from_this();
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
            auto opt = gw.parse(carrier_, buffer_, shared_this());
            if (! opt)
                return;

            carrier_.pack(buffer_);
            net::async_write(opt.value()->get(), net::buffer(buffer_), net::bind_executor(strand_,
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
        T& gw;
        socket_t socket_;
        strand_t strand_;
        buffer_t buffer_;
        carrier_t carrier_;
};
   
template <typename T>
class response : public std::enable_shared_from_this<response<T>>
{
    public:
        explicit response(T& g, net::io_context& ioc, uint32_t service) : gw(g),
        resolver_(ioc), socket_(ioc), strand_(socket_.get_executor()), service_(service)
        {
        }
    
        socket_t& get()
        {
            return socket_;
        }
    
        std::shared_ptr<response<T>> shared_this()
        {
            return this->shared_from_this();
        }
    
        void run(const std::string& host, const std::string& port)
        {
            host_ = host;
            resolver_.async_resolve(host, port, net::bind_executor(strand_,
            [self = shared_this()](error_code_t ec, results_t results)
            {
                self->on_resolve(ec, results);
            }));
        }
    
        void on_resolve(error_code_t ec, results_t results)
        {
            if (ec)
                return fail(ec, "resolve");

            net::async_connect(socket_, results,
            net::bind_executor(strand_, std::bind(&response::on_connect, shared_this(), std::placeholders::_1)));
        }
    
        void on_connect(error_code_t ec)
        {
            if (ec)
                return fail(ec, "connect");

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
            {
                gw.close(service_);
                return;
            }

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
            {
                gw.close(service_);
                return;
            }

            do_write();
        }

        void do_write()
        {
            auto opt = gw.parse(carrier_, buffer_);
            if (! opt)
                return;

            carrier_.pack(buffer_);
            net::async_write(opt.value()->get(), net::buffer(buffer_), net::bind_executor(strand_,
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
        T& gw;
        tcp::resolver resolver_;
        socket_t socket_;
        strand_t strand_;
        buffer_t buffer_;
        carrier_t carrier_;
        std::string host_;
        uint32_t service_;
};

class listener : public std::enable_shared_from_this<listener>
{
    public:
        using request_t = std::shared_ptr<request<listener>>;
        using response_t = std::shared_ptr<response<listener>>;
    
        listener(net::io_context& ioc_, endpoint_t endpoint) :
        ioc(ioc_), acceptor_(ioc)
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
    
        uint32_t next()
        {
            return sequence++;
        }
    
        void close(uint32_t service)
        {
            responses.erase(service);
        }

        std::optional<request_t> parse(carrier_t& carrier_, const buffer_t& buffer_)
        {
            auto header = carrier_.header();
            carrier_.decode_message(buffer_);
            auto it = requests.find(header->seq());
            if (it == requests.end())
                return {};
            auto ptr = it->second.second;
            header->set_seq(it->second.first);
            requests.erase(it);
            return ptr;
        }
    
        std::optional<response_t> parse(carrier_t& carrier_, const buffer_t& buffer_, request_t ptr)
        {
            auto header = carrier_.header();
            carrier_.decode_message(buffer_);
            auto it = responses.find(header->service());
            if (it == responses.end())
                return {};
            uint32_t seq = next();
            requests.try_emplace(seq, header->seq(), ptr);
            header->set_seq(seq);
            return it->second;
        }

        void run(const service_t& services)
        {
            if (! acceptor_.is_open())
                return;
            for (auto& [service, endpoint] : services)
            {
                auto resp = std::make_shared<response<listener>>(*this, ioc, service);
                responses.try_emplace(service, resp);
                resp->run(endpoint.first, endpoint.second);
            }

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
                std::make_shared<request<listener>>(*this, std::move(socket))->run();

            do_accept();
        }

    private:
        uint32_t sequence = 0;
        net::io_context& ioc;
        tcp::acceptor acceptor_;
        hashmap_t<response_t> responses;
        hashmap_t<std::pair<uint32_t, request_t>> requests;
};
    
#endif
