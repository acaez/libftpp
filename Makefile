NAME = libftpp.a

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++11

OBJDIR = obj
SOURCE = src/str/strlen.c \

OBJET = $(SOURCE:.c=.o)

RM = @rm -rf
AR = @ar -rcs

all: $(NAME)

$(NAME): $(OBJET)
	$(AR) $(NAME) $(OBJET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ -I .

clean:
	$(RM) $(OBJDIR) tests/bin

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re bonus
