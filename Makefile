CC = c++
CFLAGS = -std=c++98 -Wall -Wextra -Werror

NAME = webserv
SRCS = main.cpp ./src/config/Config.cpp ./src/utils/ConfigParsUtil.cpp
OBJS = $(SRCS:.cpp=.o)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
