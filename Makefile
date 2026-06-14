NAME		= ircserv
CPP		= c++
CPPFLAGS	= -Wall -Werror -Wextra -O3 -std=c++98 -I./incs/
SP		= src
SRC		= $(SP)/main.cpp $(SP)/Server.cpp $(SP)/Client.cpp $(SP)/Channel.cpp $(SP)/parsing/Message.cpp
OBJ		= $(SRC:$(SP)/%.cpp=$(OP)/%.o)
OP		= obj

$(OP)/%.o: $(SP)/%.cpp
	@mkdir -p $(@D)
	$(CPP) $(CPPFLAGS) -c $< -o $@

$(NAME): $(OBJ)
	$(CPP) $(CPPFLAGS) $(OBJ) -o $(NAME)

all: $(NAME)

clean:
	@echo "Deleting obj/ directory"
	rm -rf $(OP)

fclean: clean
	@echo "Deleting binary"
	rm -f $(NAME)

re: fclean all
