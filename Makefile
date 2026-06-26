CC          = c++
CXXFLAGS    = -Wall -Wextra -Werror -g -std=c++98
NAME        = Webserver
INCLUDES    = -I./include

OBJS_DIR    = ./obj

SOURCES_DIR  = ./src
SOURCES     = main.cpp
SOURCES_OBJ = $(addprefix $(OBJS_DIR)/, $(SOURCES:.cpp=.o))

SERVER_DIR  = ./src/server
SERVER_SRC  = Server.cpp Socket.cpp Epoll.cpp Client.cpp
SERVER_OBJ  = $(addprefix $(OBJS_DIR)/, $(SERVER_SRC:.cpp=.o))

CONFIG_DIR  = ./src/config
CONFIG_SRC  = Config.cpp ServerConfig.cpp LocationConfig.cpp
CONFIG_OBJ  = $(addprefix $(OBJS_DIR)/, $(CONFIG_SRC:.cpp=.o))

UTILS_DIR   = ./src/utils
UTILS_SRC   = Utils.cpp ConfigParsUtil.cpp
UTILS_OBJ   = $(addprefix $(OBJS_DIR)/, $(UTILS_SRC:.cpp=.o))

HTTP_DIR   = ./src/http
HTTP_SRC   = HttpUtils.cpp RequestParser.cpp
HTTP_OBJ   = $(addprefix $(OBJS_DIR)/, $(HTTP_SRC:.cpp=.o))

CGI_DIR   = ./src/cgi
CGI_SRC   = Cgi.cpp
CGI_OBJ   = $(addprefix $(OBJS_DIR)/, $(CGI_SRC:.cpp=.o))

OBJS        = $(SOURCES_OBJ) $(SERVER_OBJ) $(UTILS_OBJ) $(CONFIG_OBJ) $(HTTP_OBJ)

vpath %.cpp . $(SERVER_DIR) $(SOURCES_DIR) $(UTILS_DIR) $(CONFIG_DIR) $(HTTP_DIR)

all : $(NAME)

$(NAME) : $(OBJS)
	$(CC) $(CXXFLAGS) -o $@ $^

$(OBJS_DIR)/%.o : %.cpp
	@mkdir -p $(OBJS_DIR)
	$(CC) $(CXXFLAGS) $(INCLUDES) -o $@ -c $<

clean :
	rm -rf $(OBJS_DIR)

fclean : clean
	rm -f $(NAME)

re : fclean all

.PHONY : all clean fclean re