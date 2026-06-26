CC        = c++
CXXFLAGS  = -Wall -Wextra -Werror -g -std=c++98
ROOT      = ../..
INCLUDES  = -I$(ROOT)/include

NAME      = test_request_parser
SRCS      = test_request_parser.cpp $(ROOT)/src/http/RequestParser.cpp $(ROOT)/src/http/HttpUtils.cpp

all: $(NAME)
	./$(NAME)

$(NAME): $(SRCS)
	$(CC) $(CXXFLAGS) $(INCLUDES) -o $(NAME) $(SRCS)

clean:
	rm -f $(NAME)

.PHONY: all clean
