NAME = ircserv

CPP = c++
CPPFLAGS = -Wall -Werror -Wextra -O3 -std=c++98 -I./incs/
IP = incs
inc = $(IP)/master.h $(IP)/Client.h $(IP)/Server.h
SP = src
SRC = $(SP)/main.cpp $(SP)/Server.cpp
OBJ = $(SRC:.cpp=.o)
OP = obj

$(OP)/%.o: $(SP)/%.cpp | $(OP)
	@mkdir -p $(@D)
	$(CPP) $(CPPFLAGS) -c $< -o $@

$(NAME): $(OBJ)
	$(CPP) $(CPPFLAGS) -o $(NAME)

all: $(NAME)

clean:
	rm -rf $(OP)

fclean: clean
	rm -f $(NAME)
