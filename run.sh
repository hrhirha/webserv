#!/bin/bash

echo "compiling" && \
clang++ -std=c++98 -Wall -Wextra -Werror -fsanitize=address -fsanitize=undefined main.cpp Response.cpp utils.cpp && \
echo "executing" && \
./a.out | cat -e
