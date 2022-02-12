/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mlasrite <mlasrite@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/11/19 18:21:53 by mlasrite          #+#    #+#             */
/*   Updated: 2021/11/19 18:52:00 by mlasrite         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "src/Server/server.hpp"

int main(int argc, char **argv)
{
    if (argc == 2)
        entry(std::string(argv[1]));
    else if (argc == 1)
        entry("conf/webserv.conf");
    else
        return 1;
    return 0;
}