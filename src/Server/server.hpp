/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ibouhiri <ibouhiri@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/11/19 18:28:22 by mlasrite          #+#    #+#             */
/*   Updated: 2022/02/12 16:37:00 by ibouhiri         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "socket.hpp"
#include <vector>
#include <algorithm>
#include <map>
#include "../Response/Response.hpp"
#define MAX_BUFFER_SIZE 1024 * 20

class Server
{
private:
    std::vector<Socket *> _sockets;
    std::map<size_t, std::string> _ports;
    std::vector<ServerCnf> _servConf;

    fd_set _readSet;
    fd_set _writeSet;
    int _maxFd;
    std::map<int, std::pair<Request, Response> > _clients;

public:
    Server() : _maxFd(-1) {}
    ~Server() { this->clean(); }

    void setPorts(std::map<size_t, std::string> &ports) { this->_ports = ports; }
    void setServConf(std::vector<ServerCnf> &servConf) { this->_servConf = servConf; }

    void startServSockets()
    {
        // create a server socket for each port
        for (std::map<size_t, std::string>::iterator it = this->_ports.begin(); it != this->_ports.end(); it++)
        {
            Socket *sock = new Socket(true);
            sock->setPort(it->first);
            sock->setHost(it->second);
            this->_sockets.push_back(sock);
        }

        for (size_t i = 0; i < this->_sockets.size(); i++)
        {
            if (this->_sockets[i]->isServSock())
                this->_sockets[i]->launchSock();
        }
    }

    void addToSet(int fd, fd_set &set)
    {
        FD_SET(fd, &set);

        // update our maxFd if the new fd is greater than madFd
        if (fd > this->_maxFd)
            this->_maxFd = fd;
    }

    void deleteFromSet(int fd, fd_set &set) { FD_CLR(fd, &set); }

    void fillSockSet()
    {
        // reset our sets
        FD_ZERO(&this->_readSet);
        FD_ZERO(&this->_writeSet);

        // add server sockets to the read set
        for (size_t i = 0; i < this->_sockets.size(); i++)
            addToSet(this->_sockets[i]->getSockFd(), this->_readSet);
    }

    void performSelect()
    {
        fd_set tmpReadSet = this->_readSet;
        fd_set tmpWriteSet = this->_writeSet;

        // wait for events in ReadSet and WriteSet
        int result = select(this->_maxFd + 1, &tmpReadSet, &tmpWriteSet, NULL, NULL);
        if (result == -1)
            std::cout << "Error\n";
        else if (result > 0)
        {
            for (size_t i = 0; i < this->_sockets.size(); i++)
            {
                // check if a file descriptor is ready for read
                if (FD_ISSET(this->_sockets[i]->getSockFd(), &tmpReadSet))
                {
                    if (this->_sockets[i]->isServSock())
                        acceptNewClient(this->_sockets[i]);
                    else
                        handleClient(this->_sockets[i]);
                }

                // check if a file descriptor is ready for write
                if (FD_ISSET(this->_sockets[i]->getSockFd(), &tmpWriteSet))
                    sendRequest(this->_sockets[i]);
            }
        }
    }

    void acceptNewClient(Socket *sock)
    {
        std::cout << "New Client appeared on port: " << sock->getPort() << std::endl;

        // accept connection on server socket and get fd for new Client
        int newClient = accept(sock->getSockFd(), 0, 0);
        // std::cout << newClient << std::endl;
        Socket *client = new Socket(false);
        client->setSockFd(newClient);
        client->setSockAddr(sock->getSockAddr());
        Request req;
        Response res;
        this->_clients.insert(std::make_pair(newClient, std::make_pair(req, res)));
        this->_sockets.push_back(client);

        // add our client to read set
        addToSet(client->getSockFd(), this->_readSet);
    }

    void handleClient(Socket *client)
    {
        char buff[MAX_BUFFER_SIZE];
        bzero(buff, MAX_BUFFER_SIZE);
        std::cout << "Handle Client: " << client->getSockFd() << std::endl;

        // receive data from client
        int size = recv(client->getSockFd(), buff, MAX_BUFFER_SIZE - 1, 0);

        std::cout << " size -> " <<  size << std::endl;

        // if an error occured or when a stream socket peer has performed a shutdown.
        if (size == -1 || size == 0)
        {
            deleteFromSet(client->getSockFd(), this->_readSet);
            client->m_close();
            std::vector<Socket *>::iterator position = std::find(this->_sockets.begin(), this->_sockets.end(), client);
            if (position != this->_sockets.end())
            {
                delete (*position);
                this->_sockets.erase(position);
            }
            return;
        }

        // send to parser and check return value if reading is complete
        if (size > 0)
        {
            // std::string newStr = std::string(buff);
            // Request req(newStr, this->_servConf, client->getSockAddr());
            // this->_clients[client->getSockFd()].first = req;
            // bool isComplete = this->_clients[client->getSockFd()].req.isRequestCompleted();
            // std::cout << isComplete << std::endl;
            if (true)
            {
                deleteFromSet(client->getSockFd(), this->_readSet);
                addToSet(client->getSockFd(), this->_writeSet);
            }
        }
    }

    void sendRequest(Socket *client)
    {
        time_t t;
        time(&t);
        std::string body = ctime(&t);
        std::string headers = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: ";
        headers.append(std::to_string(body.size()));
        headers.append("\n\n");
        char *response = strdup((headers + body).c_str());
        send(client->getSockFd(), response, strlen(response), 0);

        // is Connection -> keep-alive
        if (client->keepAlive())
        {
            std::cout << client->getSockFd() << ": Connection -> "
                      << " keep-alive." << std::endl;
            deleteFromSet(client->getSockFd(), this->_writeSet);
            addToSet(client->getSockFd(), this->_readSet);
        }

        // else Connection -> close
        else
        {
            std::cout << client->getSockFd() << ": Connection -> "
                      << " close." << std::endl;
            deleteFromSet(client->getSockFd(), this->_writeSet);
            client->m_close();
            std::vector<Socket *>::iterator position = std::find(this->_sockets.begin(), this->_sockets.end(), client);
            if (position != this->_sockets.end())
            {
                delete (*position);
                this->_sockets.erase(position);
            }
        }
    }

    void clean()
    {
        for (size_t i = 0; i < this->_sockets.size(); i++)
        {
            this->_sockets[i]->m_close();
            delete _sockets[i];
        }
        this->_sockets.clear();
    }
};

int entry(std::string file);
