//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef FILE_TRANSFER_HPP
#define FILE_TRANSFER_HPP

#include <cstdint>
#include <fstream>
#include <filesystem>
#include <common.hpp>
#include <experimental/net>

namespace fs = std::filesystem;
namespace net = std::experimental::net;

using byte_t = char;
using tcp = net::ip::tcp;
using length_t = uint32_t;
using socket_t = tcp::socket;
using endpoint_t = tcp::endpoint;
using error_code_t = std::error_code;
using buffer_t = std::vector<byte_t>;
using results_t = tcp::resolver::results_type;
using strand_t = net::strand<net::io_context::executor_type>;

template <typename T, typename = std::void_t<>>
struct is_container : std::false_type
{
};

template <typename T>
struct is_container<
    T,
    std::void_t<
        typename T::value_type,
        typename T::size_type,
        typename T::allocator_type,
        typename T::iterator,
        typename T::const_iterator,
        decltype(std::declval<T>().size()),
        decltype(std::declval<T>().begin()),
        decltype(std::declval<T>().end()),
        decltype(std::declval<T>().cbegin()),
        decltype(std::declval<T>().cend())
    >
> : std::true_type
{
};

template <typename T>
concept bool is_container_v = is_container<T>::value;

class protocol
{
    public:
        protocol() = default;

        length_t length() const
        {
            return length_;
        }

        void set_length(length_t length)
        {
            length_ = length;
        }

        length_t tagsize() const
        {
            return tagsize_;
        }

        void set_tagsize(length_t tagsize)
        {
            tagsize_ = tagsize;
        }

    private:
        length_t length_;
        length_t tagsize_;
};

inline constexpr size_t header_size()
{
    return sizeof(protocol);
}
  
inline constexpr size_t length_size()
{
    return sizeof(length_t);
}
  
class carrier 
{
    public:
        using protocol_t = std::shared_ptr<protocol>;

        carrier() : header_(std::make_shared<protocol>()) 
        {
        }

        protocol_t header()
        {
            return header_;
        }

        void set_header(protocol_t header)
        {
            header_ = header;
        }

        length_t decode_header(buffer_t& buffer_)
        {
            size_t i = 0;
            length_t length = to_size<length_t>(buffer_, i);
            header_->set_length(length);
            header_->set_tagsize(to_size<length_t>(buffer_, i));
            buffer_.resize(header_size() + length);
            return length;
        }

        std::string decode_message(const buffer_t& buffer_, const fs::path& path = fs::path())
        {
            auto tagsize = header_->tagsize();
            std::string tag(std::addressof(buffer_[header_size()]), tagsize);
            fs::path file = path / tag;
            if (header_->length() > tagsize)
            {
                fs::create_directories(file.parent_path());
                std::ofstream ofs(file, std::ios::binary | std::ios::trunc);
                auto offset = header_size() + tagsize;
                ofs.write(std::addressof(buffer_[offset]), buffer_.size() - offset);
            }
            else if (tag[tagsize - 1] == '/')
            {
                auto dir = file.string();
                dir.resize(dir.size() - 1);
                fs::create_directories(dir);
            }
            else if (! path.empty())
            {
                fs::create_directories(file.parent_path());
                std::ofstream(file, std::ios::binary | std::ios::trunc);
            }
            return tag;
        }

        template <typename T = std::string>
        requires is_container_v<T>
        void pack(buffer_t& buffer_, const std::string& tag, const T& buffer = T())
        {
            length_t length = tag.size() + buffer.size();
            header_->set_length(length);
            header_->set_tagsize(tag.size());
            buffer_.resize(header_size() + length);
            encode_header(buffer_);
            size_t i = header_size();
            for (const auto& t : tag)
                 buffer_[i++]= t;
            for (const auto& b : buffer)
                 buffer_[i++]= b;
        }

    private:
        template <typename T>
        T to_size(const buffer_t& buffer_, size_t& i)
        {
            T size = 0;
            for (size_t j = 0; j != sizeof(T); ++j)
                 size = (size << 8) + (static_cast<T>(buffer_[i++]) & 0xFF);
            return size;
        }
            
        template <typename T, typename B>
        void to_byte(buffer_t& buffer_, size_t& i, T value)
        {
            for (size_t j = 0; j != sizeof(T); ++j)
                 buffer_[i++] = static_cast<B>((value >> ((sizeof(T) - j - 1) * 8)) & 0xFF);
        }
        
        void encode_header(buffer_t& buffer_)
        {
            size_t i = 0;
            to_byte<length_t, byte_t>(buffer_, i, header_->length());
            to_byte<length_t, byte_t>(buffer_, i, header_->tagsize());
        }

    private:
        protocol_t header_;
};

using carrier_t = carrier;

std::string dos2unix(const fs::path& path)
{
    std::string path_ = path.string();
#if defined(_WIN32)
    std::replace(path_.begin(), path_.end(), '\\', '/');
#endif
    return path_;
}

#endif
