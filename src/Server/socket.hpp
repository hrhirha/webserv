/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   socket.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ibouhiri <ibouhiri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/11/20 14:27:13 by mlasrite          #+#    #+#             */
/*   Updated: 2022/02/12 16:26:29 by ibouhiri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define QUEUE_SIZE 10

class Socket
{
private:
    int _sockfd;
    int _port;
    struct sockaddr_in _serv_addr;
    bool _isServSock;
    bool _keepAlive;
    std::string _host;

public:
    Socket(bool isServ) : _isServSock(isServ), _keepAlive(false) {}
    ~Socket() {}

    bool operator==(const Socket &a)
    {
        return (a.getSockFd() == this->getSockFd());
    }

    void launchSock()
    {
        if (this->_isServSock)
        {
            // create socket
            this->_sockfd = socket(AF_INET, SOCK_STREAM, 0);

            // assign  Ip, convert port to network byte order and assign local address
            this->_serv_addr.sin_family = AF_INET;
            this->_serv_addr.sin_port = htons(this->_port);
            if (this->_host == "localhost")
                this->_host = "127.0.0.1";
            this->_serv_addr.sin_addr.s_addr = inet_addr(this->_host.c_str());

            // asign address to socket
            bind(this->_sockfd, (struct sockaddr *)&this->_serv_addr, sizeof(this->_serv_addr));

            // prepare the server for incoming clients requests
            listen(this->_sockfd, QUEUE_SIZE);
            std::cout << "Server listening on port: " << this->_port << std::endl;
        }
    }

    bool isServSock() const { return this->_isServSock; }
    bool keepAlive() const { return this->_keepAlive; }
    void m_close() const { close(this->_sockfd); }
    void setPort(int port) { this->_port = port; }
    void setHost(std::string host) { this->_host = host; }
    struct sockaddr_in getSockAddr() { return this->_serv_addr; }
    void setSockFd(int fd) { this->_sockfd = fd; }
    void setSockAddr(struct sockaddr_in servAddr) { this->_serv_addr = servAddr; }
    int getSockFd() const { return this->_sockfd; }
    int getPort() const { return this->_port; }
};
