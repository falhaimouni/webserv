CXX = c++
CXXFLAGS = -Wall -Werror -Wextra -std=c++98
NAME = webserver
INCLUDES = -I./includes

SRCS = main.cpp \
		srcs/handlers/StaticHandler.cpp \
		srcs/handlers/ErrorHandler.cpp \
		srcs/handlers/CGIHandler.cpp \
		srcs/handlers/ResponseBuilder.cpp \
		srcs/handlers/UploadHandler.cpp \
		srcs/handlers/getType.cpp \
		srcs/handlers/RequestRouter.cpp \
		srcs/handlers/DirectoryHandler.cpp \
		srcs/routing/Router.cpp \
		srcs/http/HttpParser.cpp \
		srcs/http/ValAndHelper.cpp \
		srcs/client/ClientManager.cpp \
		srcs/server/ServerManager.cpp \
		srcs/poll/Poller.cpp \
		srcs/config/ConfigParser.cpp \
		srcs/utils/Logger.cpp
	   
OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
