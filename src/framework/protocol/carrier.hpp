//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef CARRIER_HPP
#define CARRIER_HPP

#include <memory>
#include <vector>
#include <cstddef>
#include <protocol.hpp>

using byte_t = std::byte;
using buffer_t = std::vector<byte_t>;

inline constexpr size_t header_size()
{
    return sizeof(protocol);
}
  
inline constexpr size_t length_size()
{
    return sizeof(length_t);
}
  
template <typename ProtocolBuffer>
class carrier 
{
    public:
        using protocol_t = std::shared_ptr<protocol>;
        using message_t = std::shared_ptr<ProtocolBuffer>;

        carrier() : header_(std::make_shared<protocol>()), 
        message_(std::make_shared<ProtocolBuffer>())
        {
        }

        message_t message()
        {
            return message_;
        }

        void set_message(message_t message)
        {
           message_ = message;
        }

        protocol_t header()
        {
            return header_;
        }

        void set_header(protocol_t header)
        {
            header_ = header;
        }

        length_t decode_header(buffer_t& buffer)
        {
            size_t i = 0;
            auto mark = header_->mark();
            mark[0] = std::to_integer<char>(buffer[i++]);
            mark[1] = std::to_integer<char>(buffer[i++]);
            header_->set_version(std::to_integer<uint8_t>(buffer[i++]));
            header_->set_crypt(std::to_integer<uint8_t>(buffer[i++]));
            length_t bytes_transferred = to_size<length_t>(buffer, i);
            header_->set_length(bytes_transferred);
            header_->set_mode(std::to_integer<uint8_t>(buffer[i++]));
            header_->set_type(std::to_integer<uint8_t>(buffer[i++]));
            header_->set_service(to_size<uint16_t>(buffer, i));
            header_->set_agent(to_size<uint16_t>(buffer, i));
            header_->set_error(to_size<uint16_t>(buffer, i));
            header_->set_seq(to_size<uint32_t>(buffer, i));
            header_->set_res(to_size<uint32_t>(buffer, i));
            buffer.resize(header_size() + bytes_transferred);
            return bytes_transferred;
        }

        bool decode_message(const buffer_t& buffer)
        {
            return message_->ParseFromArray(std::addressof(buffer[header_size()]), buffer.size() - header_size());
        }

        void pack(buffer_t& buffer)
        {
            length_t bytes_transferred = message_->ByteSizeLong();
            header_->set_length(bytes_transferred);
            buffer.resize(header_size() + bytes_transferred);
            encode_header(buffer);
            message_->SerializeToArray(std::addressof(buffer[header_size()]), bytes_transferred);
        }

    private:
        template <typename T>
        T to_size(const buffer_t& buffer, size_t& i)
        {
            T size = 0;
            for (size_t j = 0; j != sizeof(T); ++j)
                 size = (size << 8) + (std::to_integer<T>(buffer[i++]) & 0xFF);
            return size;
        }
            
        template <typename T, typename B>
        void to_byte(buffer_t& buffer, size_t& i, T value)
        {
            for (size_t j = 0; j != sizeof(T); ++j)
                 buffer[i++] = static_cast<B>((value >> ((sizeof(T) - j - 1) * 8)) & 0xFF);
        }
        
        void encode_header(buffer_t& buffer)
        {
            size_t i = 0;
            auto mark = header_->mark();
            buffer[i++] = static_cast<byte_t>(mark[0]);
            buffer[i++] = static_cast<byte_t>(mark[1]);
            buffer[i++] = static_cast<byte_t>(header_->version());
            buffer[i++] = static_cast<byte_t>(header_->crypt());
            to_byte<length_t, byte_t>(buffer, i, header_->length());
            buffer[i++] = static_cast<byte_t>(header_->mode());
            buffer[i++] = static_cast<byte_t>(header_->type());
            to_byte<uint16_t, byte_t>(buffer, i, header_->service());
            to_byte<uint16_t, byte_t>(buffer, i, header_->agent());
            to_byte<uint16_t, byte_t>(buffer, i, header_->error());
            to_byte<length_t, byte_t>(buffer, i, header_->seq());
            to_byte<length_t, byte_t>(buffer, i, header_->res());
        }

    private:
        protocol_t header_;
        message_t message_;
};

#endif
