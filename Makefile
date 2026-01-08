CXX = c++
CXXFLAGS = -Wall -Werror -Wextra -std=c++98
NAME = webserver
INCLUDES = -I./includes

SRCS = \
	main.cpp \
	srcs/http/HttpParser.cpp \
	srcs/http/ValAndHelper.cpp \

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
