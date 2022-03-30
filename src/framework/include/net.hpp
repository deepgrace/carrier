//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef NET_HPP
#define NET_HPP

#include <common.hpp>
#include <carrier.hpp>
#include <carrier.pb.h>
#include <experimental/net>

namespace net = std::experimental::net;
using tcp = net::ip::tcp;
using socket_t = tcp::socket;
using endpoint_t = tcp::endpoint;
using error_code_t = std::error_code;
using carrier_t = carrier<pb::carrier>;
using results_t = tcp::resolver::results_type;
using strand_t = net::strand<net::io_context::executor_type>;

#endif
