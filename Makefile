NAME = webserv
CC = clang++
FLAGS = -std=c++98 -Wall -Wextra -Werror -fsanitize=address -fsanitize=undefined -g3
SRCS = main.cpp Response.cpp res_utils.cpp

all: $(NAME)

run: $(NAME)
	@echo "Executing..."
	@./$(NAME) | cat -e

leaks: bg
	@echo "Cheking fd leaks"
	lsof -p $(shell ps | grep $(NAME) | cut -d ' ' -f 1)
	@killall $(NAME)

bg:
	@./$(NAME) &

$(NAME):
	@echo "Compiling..."
	@$(CC) $(FLAGS) $(SRCS) -o $(NAME)

clean:
	@rm -rf $(NAME)
	@rm -rf $(NAME).dSYM

re: clean all
