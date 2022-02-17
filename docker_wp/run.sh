#!/bin/bash

eval $(docker-machine env)
docker build -t webserv:0.1 .
docker run --name test -p 80:80 -it --rm webserv:0.1
