//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef ASIO_ASYNC_SSL_HPP
#define ASIO_ASYNC_SSL_HPP

#include <common.hpp>
#include <carrier.hpp>
#include <carrier.pb.h>
#include <boost/asio.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>

namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;
using endpoint_t = tcp::endpoint;
using carrier_t = carrier<pb::carrier>;
using socket_t = ssl::stream<tcp::socket>;
using results_t = tcp::resolver::results_type;
using error_code_t = boost::system::error_code;

#endif
