# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Dockerfile                                         :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: hrhirha <marvin@42.fr>                     +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2019/12/25 12:29:49 by hrhirha           #+#    #+#              #
#    Updated: 2022/02/15 21:43:15 by hrhirha          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

FROM debian:buster

COPY srcs/setup.sh .
COPY srcs/config.sh .
COPY srcs/wp_db.sql .
COPY srcs/wp /var/www/html/
COPY srcs/webserv_clone /webserv_clone
RUN sh setup.sh

CMD ["sh", "config.sh"]

EXPOSE 80 443
