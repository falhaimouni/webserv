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
	   srcs/handlers/RequestParser.cpp \
	   srcs/handlers/RequestRouter.cpp \
	   srcs/handlers/RequestHandler.cpp \
	   srcs/handlers/DirectoryHandler.cpp \
	   srcs/server/Server.cpp \
	   
OBJS = $(SRCS:srcs/%.cpp=objs/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OBJS) -o $(NAME)

objs/%.o: srcs/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf objs

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re
