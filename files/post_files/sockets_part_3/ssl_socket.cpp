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
#include "ssl_socket.h"
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <fcntl.h>
#include <thread>

namespace
{
    const ssl_socket_exception NOT_CONNECTED("Socket not connected");
}

ssl_socket::ssl_socket(const std::string & _host, const std::string & _port):
    address_info(nullptr),
    connection(-1),
    host(_host),
    port(_port)
{

}

ssl_socket::~ssl_socket()
{
    disconnect();
}

ssl_socket& ssl_socket::connect()
{
    if (connection >= 0)
    {
        throw ssl_socket_exception("Attempting to connect after socket already connected");
    }

    if (address_info == nullptr)
    {
        struct addrinfo hints = {0};
        hints.ai_family = PF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        int error = getaddrinfo(host.c_str(), port.c_str(), &hints, &address_info);
        if (error != 0)
        {
            throw ssl_socket_exception(std::string("Error getting address info: ") + std::string(gai_strerror(error)));
        }
    }

    std::string error_string = "";
    for (struct addrinfo* current_address_info = address_info; current_address_info != nullptr; current_address_info = current_address_info->ai_next)
    {
        connection = socket(current_address_info->ai_family, current_address_info->ai_socktype, current_address_info->ai_protocol);
        if (connection < 0)
        {
            error_string = "Unable to open socket: " + std::string(strerror(errno));
            continue;
        }
        
        if (::connect(connection, current_address_info->ai_addr, current_address_info->ai_addrlen) < 0)
        {
            error_string = "Unable to connect: " + std::string(strerror(errno));
            close(connection); // Cleanup
            connection = -1;
            continue;
        }
        
        if (fcntl(connection, F_SETFL, O_NONBLOCK) < 0)
        {
            error_string = "Unable to set nonblocking: " + std::string(strerror(errno));
            close(connection); // Cleanup
            connection = -1;
            continue;
        }

        break; // Success
    }
    if (connection < 0) // If we failed to connect
    {
        throw ssl_socket_exception(error_string);
    }
    return *this;
}

ssl_socket& ssl_socket::write(const uint8_t* data, size_t length)
{
    for (const uint8_t* current_position = data, * end = data + length; current_position < end; )
    {
        ssize_t sent = send(connection, current_position, end - current_position, 0);
        switch (sent)
        {
          case -1: // We got an error, check errno
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            } else {
                throw ssl_socket_exception("Error sending socket: " + std::string(strerror(errno)));
            }
            break;
          case 0: // The socket has been closed on the other end
            disconnect();
            throw ssl_socket_exception("The socket disconnected");
            break;
          default:
            current_position += sent;
            break;
        }
    }
    return *this;
}

ssl_socket& ssl_socket::write(const std::string & data)
{
    return write((uint8_t*)data.c_str(), data.size());
}

void ssl_socket::disconnect()
{
    if (connection >= 0)
    {
        close(connection);
        connection = -1;
    }
    if (address_info != nullptr)
    {
        freeaddrinfo(address_info);
        address_info = nullptr;
    }
}

size_t ssl_socket::read(char* buffer, size_t length)
{
    ssize_t read_size = recv(connection, buffer, length, 0);
    switch (read_size)
    {
      case -1:
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return 0; // read nothing
        } else {
            throw ssl_socket_exception("Error reading socket: " + std::string(strerror(errno)));
        }
        break;
      case 0: // The socket has been closed on the other end
        disconnect();
        return 0;
        break;
      default:
        return read_size;
        break;
    }
}

