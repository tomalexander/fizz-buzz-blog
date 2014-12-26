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
#include <openssl/err.h>

namespace
{
    const ssl_socket_exception NOT_CONNECTED("Socket not connected");

    std::string get_ssl_error()
    {
        return std::string(ERR_error_string(0, nullptr));
    }

    /**
     * Initializes and de-initializes the OpenSSL library. This should
     * only be instantiated and destroyed once
     */
    class openssl_init_handler
    {
      public:
        openssl_init_handler()
        {
            SSL_load_error_strings();
            SSL_library_init();
        }
        ~openssl_init_handler()
        {
            ERR_remove_state(0);
            ERR_free_strings();
            EVP_cleanup();
            CRYPTO_cleanup_all_ex_data();
            sk_SSL_COMP_free(SSL_COMP_get_compression_methods());
        }
    };
}

ssl_socket::ssl_socket(const std::string & _host, const std::string & _port):
    address_info(nullptr),
    connection(-1),
    host(_host),
    port(_port),
    ssl_handle(nullptr),
    ssl_context(nullptr)
{

}

ssl_socket::~ssl_socket()
{
    disconnect();
}

ssl_socket& ssl_socket::connect()
{
    static openssl_init_handler _ssl_init_life;

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
        if (!is_secure())
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
        } else {
            ssize_t sent = SSL_write(ssl_handle, current_position, end - current_position);
            if (sent > 0)
            {
                current_position += sent;
            } else {
                switch(SSL_get_error(ssl_handle, sent))
                {
                  case SSL_ERROR_ZERO_RETURN: // The socket has been closed on the other end
                    disconnect();
                    throw ssl_socket_exception("The socket disconnected");
                    break;
                  case SSL_ERROR_WANT_READ:
                  case SSL_ERROR_WANT_WRITE:
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    break;
                  default:
                    throw ssl_socket_exception("Error sending socket: " + get_ssl_error());
                    break;
                }
            }
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
    if (ssl_handle != nullptr)
    {
        SSL_shutdown(ssl_handle);
        SSL_free(ssl_handle);
        ssl_handle = nullptr;
    }

    if (ssl_context != nullptr)
    {
        SSL_CTX_free(ssl_context);
        ssl_context = nullptr;
    }

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

size_t ssl_socket::read(void* buffer, size_t length)
{
    if (!is_secure())
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
    } else {
        ssize_t read_size = SSL_read(ssl_handle, buffer, length);
        if (read_size > 0)
        {
            return read_size;
        } else {
            switch(SSL_get_error(ssl_handle, read_size))
            {
              case SSL_ERROR_ZERO_RETURN: // The socket has been closed on the other end
                disconnect();
                return 0;
                break;
              case SSL_ERROR_WANT_READ:
              case SSL_ERROR_WANT_WRITE:
                return 0; // Read nothing
                break;
              default:
                throw ssl_socket_exception("Error reading socket: " + get_ssl_error());
                break;
            }
        }
    }
}

ssl_socket& ssl_socket::make_secure()
{
    ssl_context = SSL_CTX_new(TLSv1_client_method());
    if (ssl_context == nullptr)
    {
        throw ssl_socket_exception("Unable to create SSL context " + get_ssl_error());
    }

    // Create an SSL handle that we will use for reading and writing
    ssl_handle = SSL_new(ssl_context);
    if (ssl_handle == nullptr)
    {
        SSL_CTX_free(ssl_context);
        ssl_context = nullptr;
        throw ssl_socket_exception("Unable to create SSL handle " + get_ssl_error());
    }

    // Pair the SSL handle with the plain socket
    if (!SSL_set_fd(ssl_handle, connection))
    {
        SSL_free(ssl_handle);
        SSL_CTX_free(ssl_context);
        ssl_handle = nullptr;
        ssl_context = nullptr;
        throw ssl_socket_exception("Unable to associate SSL and plain socket " + get_ssl_error());
    }

    // Finally do the SSL handshake
    for (int error = SSL_connect(ssl_handle); error != 1; error = SSL_connect(ssl_handle))
    {
        switch(SSL_get_error(ssl_handle, error))
        {
          case SSL_ERROR_WANT_READ:
          case SSL_ERROR_WANT_WRITE:
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            break;
          default:
            SSL_free(ssl_handle);
            SSL_CTX_free(ssl_context);
            ssl_handle = nullptr;
            ssl_context = nullptr;
            throw ssl_socket_exception("Error in SSL handshake: " + get_ssl_error());
            break;
        }
    }
 
    return *this;
}
