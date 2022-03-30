//
// Copyright (c) 2016-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/carrier
//

#ifndef FILE_TRANSFER_CLIENT_ASYNC_HPP
#define FILE_TRANSFER_CLIENT_ASYNC_HPP

#include <deque>
#include <file_transfer.hpp>

class session
{
    public:
        session(net::io_context& ioc, const std::string& host, const std::string& port) :
        ioc_(ioc), socket_(ioc), resolver_(ioc)
        {
            auto endpoints = resolver_.resolve(host, port);
            do_connect(endpoints);
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
            ++nth;
            net::post(ioc_,
            [this, file, tag]
            {
                if constexpr(std::is_same_v<T, fs::path>)
                {
                    buffer_t buff_;
                    auto tag_ = to_buffer(buff_, file, tag);
                    do_write(buff_, tag_);
                }
                else
                    do_write(file, tag);
            });
        }

    private:
        std::string to_buffer(buffer_t& buffer, const fs::path& file, const std::string& path)
        {
            auto path_ = path;
            if (fs::is_regular_file(file))
            {
                buffer.resize(fs::file_size(file));
                if (! buffer.empty())
                {
                    std::ifstream ifs(file, std::ios::binary);
                    ifs.read(buffer.data(), buffer.size());
                }
            }
            else if (fs::is_directory(file))
                path_.append(1, '/');
            return path_;
        }

        void do_connect(const results_t& endpoints)
        {
            net::async_connect(socket_, endpoints,
            [this](error_code_t ec, endpoint_t)
            {
                on_connect(ec);
            });
        }   

        void on_connect(error_code_t ec)
        {
            if (! ec)
                do_read_header();
            else
            {
                fail(ec, "connect");
                socket_.close();
            }
        }

        void do_read_header()
        {
            buffer_.resize(header_size());
            net::async_read(socket_, net::buffer(buffer_), 
            [this](error_code_t ec, size_t bytes_transferred)
            {
                on_read_header(ec, bytes_transferred);
            });
        }

        void on_read_header(error_code_t ec, size_t bytes_transferred)
        {
            if (! ec)
                do_read_message();
            else
            {
                fail(ec, "read");
                socket_.close();
            }
        }

        void do_read_message()
        {
            size_t length = receiver_.decode_header(buffer_);
            net::async_read(socket_, net::buffer(std::addressof(buffer_[header_size()]), length),
            [this](error_code_t ec, size_t bytes_transferred)
            {
                on_read_message(ec, bytes_transferred);
            });
        }

        void on_read_message(error_code_t ec, size_t bytes_transferred)
        {
            if (! ec)
            {
                std::cout << receiver_.decode_message(buffer_) << std::endl;
                if (--nth)
                    do_read_header();
                else
                    socket_.close();
            }
            else
            {
                fail(ec, "read");
                socket_.close();
            }
        }

        template <typename T>
        void do_write(const T& buffer, const std::string& tag)
        {
            buffer_t buff;
            sender_.pack(buff, tag, buffer);
            bool write_in_progress = ! buffers_.empty();
            buffers_.push_back(buff);
            if (! write_in_progress)
                do_write();
        }

        void do_write()
        {
            net::async_write(socket_, net::buffer(buffers_.front()),
            [this](error_code_t ec, size_t bytes_transferred)
            {
                if (! ec)
                {
                    buffers_.pop_front();
                    if (! buffers_.empty())
                        do_write();
                }
                else
                {
                    fail(ec, "write");
                    socket_.close();
                }
            });
        }

    private:
        size_t nth = 0;
        buffer_t buffer_;
        carrier_t sender_;
        carrier_t receiver_;
        net::io_context& ioc_;
        socket_t socket_;
        tcp::resolver resolver_;
        std::deque<buffer_t> buffers_;
};

#endif
