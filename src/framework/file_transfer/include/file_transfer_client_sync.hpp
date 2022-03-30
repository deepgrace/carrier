//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef FILE_TRANSFER_CLIENT_SYNC_HPP
#define FILE_TRANSFER_CLIENT_SYNC_HPP

#include <file_transfer.hpp>

class session
{
    public:
        session(net::io_context& ioc, const std::string& host, const std::string& port) :
        socket_(ioc), resolver_(ioc)
        {
            auto endpoints = resolver_.resolve(host, port);
            net::connect(socket_, endpoints);
        } 

        void transfer(const std::string& path)
        {
            auto file = dos2unix(fs::canonical(path));
            if (file.size() > 1 && file[file.size() -1] == '/')
                file.resize(file.size() - 1);

            if (! fs::exists(file))
            {
                std::cerr << "cannot access '" << file << "': No such file or directory\n";
                exit(1);
            }

            auto root = fs::path(file);
            if (fs::is_regular_file(root))
                transfer(root, root.filename());
            else if (fs::is_directory(root))
            {
                auto parent = root.parent_path();
                if (fs::is_empty(root))
                    transfer(root, fs::relative(root, parent));
                for (const auto& d : fs::recursive_directory_iterator(root))
                {
                    auto file_ = dos2unix(d.path());
                    if (! fs::is_directory(file_) || fs::is_empty(file_))
                        transfer(file_, fs::relative(file_, parent));
                }
            }
        }

        void transfer(const fs::path& path)
        {
            transfer(std::string(), path.string());
        }

        template <typename T>
        void transfer(const T& file, const std::string& tag)
        {
            if constexpr(std::is_same_v<T, fs::path>)
            {
                auto tag_ = to_buffer(file, tag);
                carrier_.pack(buffer_, tag_, data_);
            }
            else
                carrier_.pack(buffer_, tag, file);

            net::write(socket_, net::buffer(buffer_));

            buffer_.resize(header_size());
            net::read(socket_, net::buffer(buffer_));

            auto length = carrier_.decode_header(buffer_);
            net::read(socket_, net::buffer(std::addressof(buffer_[header_size()]), length));
            std::cout << carrier_.decode_message(buffer_) << std::endl;
        }

    private:
        std::string to_buffer(const fs::path& file, const std::string& path)
        {
            data_.clear();
            auto path_ = path;
            if (fs::is_regular_file(file))
            {
                data_.resize(fs::file_size(file));
                if (! data_.empty())
                {
                    std::ifstream ifs(file, std::ios::binary);
                    ifs.read(data_.data(), data_.size());
                }
            }
            else if (fs::is_directory(file))
                path_.append(1, '/');
            return path_;
        }

    private:
        tcp::socket socket_;
        tcp::resolver resolver_;
        carrier_t carrier_;
        buffer_t data_;
        buffer_t buffer_;
};

#endif
