//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef WEBSOCKET_HPP
#define WEBSOCKET_HPP

#include <common.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <carrier.pb.h>
    
namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;

template <typename T>
using websocket_t = websocket::stream<T>;

using tcp = net::ip::tcp;
using endpoint_t = tcp::endpoint;
using buffer_t = beast::multi_buffer;
using results_t = tcp::resolver::results_type;
using error_code_t = boost::system::error_code;
   
inline std::string to_string(const buffer_t& buffer_)
{
    return beast::buffers_to_string(buffer_.data());
}

inline void to_buffer(const pb::carrier& carrier_, buffer_t& buffer_)
{
    auto str = carrier_.SerializeAsString();
    buffer_.consume(buffer_.size());
    size_t n = buffer_copy(buffer_.prepare(str.size()), net::buffer(str));
    buffer_.commit(n);
}

inline void clear(pb::carrier& carrier_, buffer_t& buffer_)
{
    carrier_.Clear();
    buffer_.consume(buffer_.size());
}

template <typename T>
void setup_stream(websocket_t<T>& ws)
{
    websocket::permessage_deflate pmd;
    pmd.memLevel = 9;
    pmd.compLevel = 9;
    pmd.client_enable = true;
    pmd.server_enable = true;

    ws.binary(true);
    ws.set_option(pmd);
    ws.auto_fragment(false);
    ws.write_buffer_bytes(8192);
    ws.read_message_max(64 * 1024 * 1024);
}

#endif
