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
#include <openssl/ssl.h>

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

/**
 * Unified interface for non-blocking read and blocking write, plain
 * and SSL sockets
 */
class ssl_socket
{
  public:
    /**
     * Construct a socket that will eventually connect to the given
     * host and port.
     * 
     * @param _host The hostname or ip address to connect to (ex: "fizz.buzz" or "208.113.196.82")
     * @param  _port The port or service name to connect to (ex: "80" or "http")
     */
    ssl_socket(const std::string & _host, const std::string & _port);
    virtual ~ssl_socket();
    ssl_socket(ssl_socket const&) = delete;
    ssl_socket& operator=(ssl_socket const&) = delete;

    /**
     * Perform a DNS request and establish an unencrypted TCP socket
     * to the host.
     * 
     * @return A reference to itself
     * @throw ssl_socket_exception if any part of the connection fails
     */
    ssl_socket& connect();

    /**
     * Disconnect from the host and destroy the socket
     */
    void disconnect();

    /**
     * Blocking write of data to the socket
     * 
     * @param data pointer to raw bytes to write to socket
     * @param length number of bytes we wish to write to the socket
     * 
     * @return a reference to itself
     * @throw ssl_socket_exception if an error occurs other than EAGAIN/EWOULDBLOCK
     */
    ssl_socket& write(const uint8_t* data, size_t length);

    /**
     * Blocking write of a string to the socket (*does not write the
     * null terminator*)
     * 
     * @param data a string to write to the socket
     * 
     * @return a reference to itself
     * @throw ssl_socket_exception if an error occurs other than EAGAIN/EWOULDBLOCK
     */
    ssl_socket& write(const std::string & data);

    /**
     * Non-blocking attempt to read from the socket
     * 
     * @param buffer a block of memory in which the read data will be placed
     * @param length the maximum number of bytes we can read into buffer
     * 
     * @return The number of bytes read from the socket. Please note that 0 can be returned if theres no data available OR if the socket has closed. Use is_connected to determine if the socket is still open.
     * @throw ssl_socket_exception if an error occurs other than EAGAIN/EWOULDBLOCK
     */
    size_t read(void* buffer, size_t length);

    /**
     * Check to see if the socket is still connected. If the socket
     * has been disconnected on the server side and no read or write
     * has occurred then it is possible for this to return true
     * because the disconnect has not yet been detected
     */
    bool is_connected() const { return connection >= 0; }

    /**
     * Check to see if this socket is an encrypted socket
     */
    bool is_secure() const { return ssl_handle != nullptr; }

    /**
     * Perform the SSL handshake to switch all communications over
     * this socket from unencrypted to encrypted
     */
    ssl_socket& make_secure();

  private:
    struct addrinfo* address_info;
    SSL* ssl_handle;
    SSL_CTX* ssl_context;
    int connection;
    std::string host;
    std::string port;
};
