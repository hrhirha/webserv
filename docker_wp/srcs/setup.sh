# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    setup.sh                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: hrhirha <marvin@42.fr>                     +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2019/12/25 12:29:54 by hrhirha           #+#    #+#              #
#    Updated: 2022/02/15 21:43:01 by hrhirha          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

#!/bin/bash

apt-get update && apt-get full-upgrade -y
apt-get install -y lsb-release gnupg php php-cgi php-fpm php-mysql wget ssh curl make sudo git mariadb-server net-tools clang

#install mysql
#echo "INSTALLING MYSQL..."
#wget http://repo.mysql.com/mysql-apt-config_0.8.13-1_all.deb
#echo "mysql-apt-config mysql-apt-config/select-server select mysql-5.7" | debconf-set-selections
#DEBIAN_FRONTEND=noninteractive  dpkg -i mysql-apt-config_0.8.13-1_all.deb
#rm mysql-apt-config_0.8.13-1_all.deb
#apt-get update
#DEBIAN_FRONTEND=noninteractive apt-get install -q -y mysql-server
echo "mysql-apt-config mysql-apt-config/root_password password mysql" | debconf-set-selections
echo "mysql-apt-config mysql-apt-config/root_password_again password mysql" | debconf-set-selections

#install PhpMyAdmin
echo "INSTALLING PHPMYADMIN..."
wget https://files.phpmyadmin.net/phpMyAdmin/4.9.0.1/phpMyAdmin-4.9.0.1-all-languages.tar.gz
tar -zxf phpMyAdmin-4.9.0.1-all-languages.tar.gz
rm phpMyAdmin-4.9.0.1-all-languages.tar.gz
mv phpMyAdmin-4.9.0.1-all-languages /var/www/html/pma
chown -R www-data:www-data /var/www/html/*

#mysql config
echo "CONFIGURING DATABASE..."
chown -R mysql: /var/lib/mysql
service mysql start
mysql -uroot -pmysql -e "GRANT ALL ON *.* TO 'admin'@'localhost' IDENTIFIED BY 'admin';FLUSH PRIVILEGES;"
mysql -uroot -pmysql -e "CREATE DATABASE wp_db;use wp_db"
#mysql -uroot -pmysql wp_db < wp_db.sql

useradd --groups sudo admin
echo "admin:admin" | chpasswd

#ssl config
echo "CONFIGURING SSL PROTOCOL..."
openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout /etc/ssl/private/nginx-selfsigned.key -out /etc/ssl/certs/nginx-selfsigned.crt -subj "/C=/ST=/L=/O=/OU=/CN="
