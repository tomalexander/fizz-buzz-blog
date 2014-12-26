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
#include <iostream>
#include <thread>
#include "ssl_socket.h"

namespace
{
    const char HOST[] = "fizz.buzz";
    const size_t BUFFER_SIZE = 1024;
}

int main(int argc, char** argv)
{
    try
    {
        ssl_socket s(HOST, "http");
        char buffer[BUFFER_SIZE];
        std::string http_query = "GET / HTTP/1.1\r\n"    \
            "Host: " + std::string(HOST) + "\r\n\r\n";

        s.connect().write(http_query);
        while (s.is_connected())
        {
            size_t length = s.read(buffer, BUFFER_SIZE);
            if (length == 0 && s.is_connected())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            } else {
                std::cout << std::string(buffer, length);
            }
        }
    } catch (const ssl_socket_exception & e) {
        std::cerr << e.to_string() << '\n';
        return 1;
    }
    
    return 0;
}
