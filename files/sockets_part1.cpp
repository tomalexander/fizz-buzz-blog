#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netdb.h>

#include <sstream>
#include <iomanip>
#include <arpa/inet.h>

namespace
{
    const char HOST[] = "fizz.buzz";
    const size_t BUFFER_SIZE = 1024;

    std::string dump_hex(unsigned char * data, size_t length)
    {
        std::stringstream out;
        for (size_t i = 0; i < length; ++i)
        {
            if (i != 0)
                out << " ";
            out << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)data[i];
        }
        return out.str();
    }

    std::string dump_ipv4_address(struct in_addr address)
    {
        char str[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &address, str, INET_ADDRSTRLEN) == nullptr)
            return "error decoding address";
        return std::string(str);
    }

    std::string dump_ipv6_address(struct in6_addr address)
    {
        char str[INET6_ADDRSTRLEN];
        if (inet_ntop(AF_INET6, &address, str, INET6_ADDRSTRLEN) == nullptr)
            return "error decoding address";
        return std::string(str);
    }

    void print_sockaddr_ipv4(struct sockaddr_in* socket_address, std::ostream & out = std::cout)
    {
        out << "sockaddr_in\n";
        out << "    sin_family\t" << socket_address->sin_family << "\n";
        out << "    sin_port\t" << ntohs(socket_address->sin_port) << "\n";
        out << "    sin_addr\t" << dump_ipv4_address(socket_address->sin_addr) << "\n";
    }

    void print_sockaddr_ipv6(struct sockaddr_in6* socket_address, std::ostream & out = std::cout)
    {
        out << "sockaddr_in6\n";
        out << "    sin6_family\t" << socket_address->sin6_family << "\n";
        out << "    sin6_port\t" << socket_address->sin6_port << "\n";
        out << "    sin6_flowinfo\t" << socket_address->sin6_flowinfo << "\n";
        out << "    sin6_addr\t" << dump_ipv6_address(socket_address->sin6_addr) << "\n";
        out << "    sin6_scope_id\t" << socket_address->sin6_scope_id << "\n";
    }

    void print_sockaddr(struct sockaddr* socket_address, std::ostream & out = std::cout)
    {
        if (socket_address == nullptr) {
            out << "\n";
            return;
        }
        
        switch(socket_address->sa_family)
        {
          case AF_INET:
            print_sockaddr_ipv4((struct sockaddr_in *)socket_address, out);
            break;
          case AF_INET6:
            print_sockaddr_ipv6((struct sockaddr_in6 *)socket_address, out);
            break;
          default:
            out << "sockaddr\n";
            out << "    sa_family\t" << socket_address->sa_family << "\n";
            out << "    sa_data\t" << dump_hex((unsigned char *)socket_address->sa_data, 14) << "\n";
            break;
        }
    }

    void print_addrinfo(struct addrinfo* address_info, std::ostream & out = std::cout)
    {
        if (address_info == nullptr)
            return;

        out << "addrinfo:\n";
        out << "  ai_flags\t" << address_info->ai_flags << "\n";
        out << "  ai_family\t" << address_info->ai_family << "\n";
        out << "  ai_socktype\t" << address_info->ai_socktype << "\n";
        out << "  ai_protocol\t" << address_info->ai_protocol << "\n";
        out << "  ai_addrlen\t" << address_info->ai_addrlen << "\n";
        out << "  ai_addr\t";
        print_sockaddr(address_info->ai_addr);
        out << "  ai_canonname\t" << address_info->ai_canonname << "\n";
        print_addrinfo(address_info->ai_next);
    }
}

int main(int argc, char** argv)
{
    struct addrinfo* address_info;
    struct addrinfo hints = {0};
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int error = getaddrinfo(HOST, "http", &hints, &address_info);
    if (error != 0)
    {
        throw std::string("Error getting address info: ") + std::string(gai_strerror(error));
    }
    print_addrinfo(address_info);
    return 0;

    int connection = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);
    if (!connection)
    {
        throw std::string("Unable to open socket");
    }
    
    // Establish a connection
    error = connect(connection, address_info->ai_addr, address_info->ai_addrlen);
    if (error == -1)
    {
        throw std::string("Unable to connect");
    }
    
    std::string http_query = "GET / HTTP/1.1\r\n"    \
        "Host: " + std::string(HOST) + "\r\n\r\n";

    char buffer[BUFFER_SIZE];
    
    send(connection, http_query.c_str(), http_query.size(), 0);
    for (ssize_t read_size = recv(connection, buffer, BUFFER_SIZE, 0);
         read_size > 0;
         read_size = recv(connection, buffer, BUFFER_SIZE, 0))
    {
        std::cout << std::string(buffer, read_size);
    }
    return 0;
}
