CC = c++
CFLAGS = -std=c++98 -Wall -Wextra -Werror
CPPFLAGS = -I./include

NAME = webserv

SRCS = main.cpp \
       src/config/Config.cpp \
       src/config/ServerConfig.cpp \
       src/config/LocationConfig.cpp \
       src/utils/ConfigParsUtil.cpp

OBJS = $(SRCS:.cpp=.o)

%.o: %.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(OBJS) -o $(NAME)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re