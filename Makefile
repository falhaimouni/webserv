NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

INCLUDES = -Iinclude

SRC = \
	src/main.cpp \
	src/server/ServerManager.cpp \
	src/poll/Poller.cpp \
	src/config/ConfigParser.cpp \
	src/utils/Logger.cpp \
	src/http/HttpHandler.cpp


OBJ = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all
