/*
Copyright (c) 2014, Tom Alexander <tom@fizz.buzz>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
#pragma once
#include <string>
#include <cinttypes>
#include <tuple>

class ssl_socket_exception
{
  public:
    ssl_socket_exception(const std::string & _msg):
        msg(_msg)
    {}

    const std::string& to_string() const {return msg;}

  private:
    std::string msg;
};

class ssl_socket
{
  public:
    ssl_socket(const std::string & _host, const std::string & port);
    virtual ~ssl_socket();
    ssl_socket(ssl_socket const&) = delete;
    ssl_socket& operator=(ssl_socket const&) = delete;

    ssl_socket& connect();
    void disconnect();
    ssl_socket& write(const uint8_t* data, size_t length);
    ssl_socket& write(const std::string & data);
    size_t read(char* buffer, size_t length);

    bool is_connected() { return connection >= 0; }

  private:
    struct addrinfo* address_info;
    int connection;
    std::string host;
    std::string port;
};

