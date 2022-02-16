# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    config.sh                                          :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: hrhirha <marvin@42.fr>                     +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2019/12/25 12:29:59 by hrhirha           #+#    #+#              #
#    Updated: 2022/02/15 16:11:19 by hrhirha          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

#!/bin/bash

#change owner
echo "CHANGING OWNERSHIP..."
chown -R mysql: /var/lib/mysql

#start services
echo "STARTING SERVICES..."
service mysql start

/bin/bash
