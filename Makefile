NAME = webserv
CC = clang++
FLAGS = -std=c++98 -Wall -Wextra -Werror #-fsanitize=address -fsanitize=undefined -g
SRCS = main.cpp src/*/*.cpp

all: $(NAME)

$(NAME):
	@$(CC) $(FLAGS) $(SRCS) -o $(NAME)

clean:
	@rm -rf $(NAME)
	@rm -rf $(NAME).dSYM

re: clean all
