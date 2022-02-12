/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mlasrite <mlasrite@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/11/19 18:51:55 by mlasrite          #+#    #+#             */
/*   Updated: 2021/11/22 12:44:19 by mlasrite         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

int entry(const std::string file)
{
    Server server;
    ServerCnf servConf(file);
    std::vector<ServerCnf> srvs = servConf.getServers();
    std::set<size_t> tmp;
    for (size_t i = 0; i < srvs.size(); i++)
        tmp.insert(srvs[i].getPort());
    std::vector<size_t> ports(tmp.begin(), tmp.end());
    server.setPorts(ports);
    server.setServConf(srvs);
    server.startServSockets();
    server.fillSockSet();

    while (1)
        server.performSelect();
    server.clean();
    return 0;
}