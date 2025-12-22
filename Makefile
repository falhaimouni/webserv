CXX = c++
CXXFLAGS = -Wall -Werror -Wextra -std=c++98
NAME = webserver
INCLUDES = -I./includes

SRCS = srcs/response.cpp srcs/handlers.cpp srcs/autoindex.cpp srcs/main_test.cpp
OBJS = $(SRCS:srcs/%.cpp=objs/%.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OBJS) -o $(NAME)

objs/%.o: srcs/%.cpp
	@mkdir -p objs
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf objs

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re
