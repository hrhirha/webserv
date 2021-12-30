#!/bin/bash

clang++ -std=c++98 -Wall -Wextra -Werror -fsanitize=address -fsanitize=undefined main.cpp Response.cpp utils.cpp && ./a.out | cat -e
