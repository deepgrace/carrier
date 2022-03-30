//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef LOAD_CONFIG_HPP
#define LOAD_CONFIG_HPP

#include <string>
#include <fstream>
#include <unordered_map>
    
template <typename T = uint32_t>
using hashmap_t = std::unordered_multimap<uint32_t, T>;

using pair_t = std::pair<std::string, std::string>;
using service_t = std::unordered_map<uint32_t, pair_t>;
   
service_t loadconfig(const std::string& file)
{
    service_t services;
    std::ifstream ifs(file);

    for (std::string line; std::getline(ifs, line);)
    {
         if (line[0] == '#' || line.size() < 11)
             continue;

         auto l = line.find_first_of(' ');
         auto r = line.find_first_not_of(' ', l + 1);

         auto m = line.find_first_of(' ', r + 1);
         auto n = line.find_first_not_of(' ', m + 1);

         auto service = std::stoul(line.substr(0, l));

         auto host = line.substr(r, m - r);
         auto port = line.substr(n);

         services.try_emplace(service, host, port);
    }

    return services;
}

#endif
