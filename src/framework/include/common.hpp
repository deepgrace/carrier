//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef COMMON_HPP
#define COMMON_HPP

#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <algorithm>
#include <functional>
#include <system_error>

template <typename E>
inline void fail(const E& ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

#endif
