SRCS		= 

OBJS		= ${SRCS:.cpp=.o}
DEPS		= ${SRCS:.cpp=.d}

CXX			= c++

CXXFLAGS	= -std=c++98 -Wall -Wextra -Werror
DEPFLAGS	= -MMD -MP

NAME		= ft_irc

all			: ${NAME}

-include ${DEPS}

%.o			: %.cpp
			${CXX} ${CXXFLAGS} ${DEPFLAGS} $< -c -o $@

${NAME}		: ${OBJS}
			${CXX} ${CXXFLAGS} ${OBJS} -o ${NAME}

clean		:
			rm -f ${OBJS} ${DEPS}

fclean		: clean
			rm -f ${NAME}

re			: fclean all

.PHONY		: all clean fclean re