//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef WEBSOCKET_GATEWAY_ASYNC_SSL_HPP
#define WEBSOCKET_GATEWAY_ASYNC_SSL_HPP

#include <root_certificates.hpp>
#include <server_certificate.hpp>
#include <websocket_async_ssl.hpp>
#include <load_config.hpp>

template <typename T>
class request : public std::enable_shared_from_this<request<T>>
{
    public:
        request(T& g, tcp::socket socket, ssl::context& ctx) : gw(g),
        ws_(std::move(socket), ctx)
        {
            setup_stream(ws_);
        }
        
        socket_t& get()
        {
            return ws_;
        }
    
        std::shared_ptr<request<T>> shared_this()
        {
            return this->shared_from_this();
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
                res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-gateway-async-ssl");
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
            auto opt = gw.parse(carrier_, buffer_, shared_this());
            if (! opt)
                return;

            auto& ws = opt.value()->get();
            to_buffer(carrier_, buffer_);
            ws.async_write(buffer_.data(),
            [self = shared_this()](error_code_t ec, size_t bytes_transferred)
            {
                self->on_write(ec, bytes_transferred);
            });
        }
    
        void on_write(error_code_t ec, size_t bytes_transferred)
        {
            if (ec)
                return fail(ec, "write");

            clear(carrier_, buffer_);
            do_read();
        }

    private:
        T& gw;
        socket_t ws_;
        buffer_t buffer_;
        pb::carrier carrier_;
};
    
template <typename T>
class response : public std::enable_shared_from_this<response<T>>
{
    public:
        explicit
        response(T& g, net::io_context& ioc, ssl::context& ctx, uint32_t service) : gw(g),
        resolver_(net::make_strand(ioc)), ws_(net::make_strand(ioc), ctx), service_(service)
        {
            setup_stream(ws_);
        }
    
        socket_t& get()
        {
            return ws_;
        }
    
        std::shared_ptr<response<T>> shared_this()
        {
            return this->shared_from_this();
        }
    
        void run(const std::string& host, const std::string& port)
        {
            host_ = host;
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

            beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

            beast::get_lowest_layer(ws_).async_connect(results,
            [self = shared_this()](error_code_t ec, results_t::endpoint_type results)
            {
                self->on_connect(ec, results);
            });
        }
    
        void on_connect(error_code_t ec, results_t::endpoint_type)
        {
            if (ec)
                return fail(ec, "connect");

            beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

            ws_.next_layer().async_handshake(ssl::stream_base::client,
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

            ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
            ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-gateway-async-ssl");
            }));

            ws_.async_handshake(host_, "/",
            [self = shared_this()](error_code_t ec)
            {
                self->on_handshake(ec);
            });
        }
    
        void on_handshake(error_code_t ec)
        {
            if (ec)
                return fail(ec, "handshake");

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

            auto& ws = opt.value()->get();
            to_buffer(carrier_, buffer_);
            ws.async_write(buffer_.data(),
            [self = shared_this()](error_code_t ec, size_t bytes_transferred)
            {
                self->on_write(ec, bytes_transferred);
            });
        }
    
        void on_write(error_code_t ec, size_t bytes_transferred)
        {
            if (ec)
                return fail(ec, "write");

            clear(carrier_, buffer_);
            do_read();
        }

    private:
        T& gw;
        tcp::resolver resolver_;
        socket_t ws_;
        buffer_t buffer_;
        pb::carrier carrier_;
        std::string host_;
        uint32_t service_;
};
    
class listener : public std::enable_shared_from_this<listener>
{
    public:
        using request_t = std::shared_ptr<request<listener>>;
        using response_t = std::shared_ptr<response<listener>>;
    
        listener(net::io_context& ioc, endpoint_t endpoint) : ioc_(ioc), acceptor_(ioc),
        ctx_server_(ssl::context::sslv23), ctx_client_(ssl::context::sslv23_client)
        {
            load_server_certificate(ctx_server_);
            load_root_certificates(ctx_client_);

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

        std::optional<request_t> parse(pb::carrier& carrier_, const buffer_t& buffer_)
        {
            if (! carrier_.ParseFromString(to_string(buffer_)))
                return {};
            auto it = requests.find(carrier_.seq());
            if (it == requests.end())
                return {};
            auto ptr = it->second.second;
            carrier_.set_seq(it->second.first);
            requests.erase(it);
            return ptr;
        }
    
        std::optional<response_t> parse(pb::carrier& carrier_, const buffer_t& buffer_, request_t ptr)
        {    
            if (! carrier_.ParseFromString(to_string(buffer_)))
                return {};
            auto it = responses.find(carrier_.service());
            if (it == responses.end())
                return {};
            uint32_t seq = next();
            requests.try_emplace(seq, carrier_.seq(), ptr);
            carrier_.set_seq(seq);
            return it->second;
        }

        void run(const service_t& services)
        {
            if (! acceptor_.is_open())
                return;
            for (auto& [service, endpoint] : services)
            {
                auto resp = std::make_shared<response<listener>>(*this, ioc_, ctx_client_, service);
                responses.try_emplace(service, resp);
                resp->run(endpoint.first, endpoint.second);
            }

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
                std::make_shared<request<listener>>(*this, std::move(socket), ctx_server_)->run();

            do_accept();
        }

    private:
        uint32_t sequence = 0;
        net::io_context& ioc_;
        tcp::acceptor acceptor_;
        ssl::context ctx_server_;
        ssl::context ctx_client_;
        hashmap_t<response_t> responses;
        hashmap_t<std::pair<uint32_t, request_t>> requests;
};

#endif
