//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <cstdint>

using length_t = uint32_t;

class protocol
{
    public:

        protocol() = default;

        char* mark()
        {
            return mark_;
        }

        void set_mark(char T, char M)
        {
            mark_[0] = T;
            mark_[1] = M;
        }

        uint8_t version() const
        {
            return version_;
        }

        void set_version(uint8_t version)
        {
            version_ = version;
        }

        uint8_t crypt() const
        {
            return crypt_;
        }

        void set_crypt(uint8_t crypt)
        {
            crypt_ = crypt;
        }

        uint32_t length() const
        {
            return length_;
        }

        void set_length(uint32_t length)
        {
            length_ = length;
        }

        uint8_t mode() const
        {
            return mode_;
        }

        void set_mode(uint8_t mode)
        {
            mode_ = mode;
        }

        uint8_t type() const
        {
            return type_;
        }

        void set_type(uint8_t type)
        {
            type_ = type;
        }

        uint16_t service() const
        {
            return service_;
        }

        void set_service(uint16_t service)
        {
            service_ = service;
        }

        uint16_t agent() const
        {
            return agent_;
        }

        void set_agent(uint16_t agent)
        {
            agent_ = agent;
        }

        uint16_t error() const
        {
            return error_;
        }

        void set_error(uint16_t error)
        {
            error_ = error;
        }

        uint32_t seq() const
        {
            return seq_;
        }

        void set_seq(uint32_t seq)
        {
            seq_ = seq;
        }

        uint32_t res() const
        {
            return res_;
        }

        void set_res(uint32_t res)
        {
            res_ = res;
        }

    private:
        char       mark_[2];   // debug prefix
        uint8_t    version_;   // protocol version
        uint8_t    crypt_;     // encrypt algorithm 
        uint32_t   length_;    // message length 
        uint8_t    mode_;      // request | response | notify
        uint8_t    type_;      // protobuf | json | tlv
        uint16_t   service_;   // login | payment | transfer 
        uint16_t   agent_;     // Android | IOS | PC | Web
        uint16_t   error_;     // error type
        uint32_t   seq_;       // message sequence number
        uint32_t   res_;       // reserved for future use
};

#endif
