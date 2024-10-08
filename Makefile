NAME		:= app
RUN_CMD		:= ./$(NAME) 

CFLAGS		+= -Wall -Wextra -std=c++11
# CFLAGS		+= -Werror
CFLAGS		+= -g -fsanitize=address

IFLAGS		:=	-Iinclude

SRC_DIR		:=	src
SRC_ENTRY	:=	main.cpp
SRCS		:=	Server.cpp


HEADER_DIR	:=	include
HEADERS 	:=	Server.hpp

OBJ_DIR		:=	obj


SRCS 		:=	$(addprefix $(SRC_DIR)/, $(SRCS))
HEADERS 	:=	$(addprefix $(HEADER_DIR)/, $(HEADERS))


OBJS 		:=	$(patsubst $(SRC_DIR)%.c, $(OBJ_DIR)%.o, $(SRCS))
OBJ_DIRS 	:=	$(dir $(OBJS))

.PHONY: make_libs fclean

all: 
	$(MAKE) $(NAME) -j4

$(NAME): $(OBJS) $(SRC_DIR)/$(SRC_ENTRY)
	$(CXX) $(SRC_DIR)/$(SRC_ENTRY) $(OBJS) $(CFLAGS) $(IFLAGS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@mkdir -p $(OBJ_DIRS)
	$(CXX) $(CFLAGS) $(IFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)
	rm -rf $(NAME).dSYM

re: fclean all

run: all
	$(RUN_CMD)

compile_commands: fclean
	$(MAKE) | compiledb

norm:
	norminette include src

