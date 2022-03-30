//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef WEBSOCKET_ASYNC_SSL_HPP
#define WEBSOCKET_ASYNC_SSL_HPP

#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <websocket.hpp>

namespace ssl = net::ssl;
using socket_t = websocket_t<beast::ssl_stream<beast::tcp_stream>>;

#endif
