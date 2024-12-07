EXEC_NAME := a.out

SOURCE := $(wildcard src/*.c)

CC := gcc

CFLAGS += -I./src
CFLAGS += -Wall -Wextra -Wshadow -Wconversion -pedantic 
CFLAGS += -ggdb3
CFLAGS += -fsanitize=undefined
CFLAGS += -fsanitize=address

LD_FLAGS += -fsanitize=undefined
LD_FLAGS += -fsanitize=address

.PHONY: all clean test

all: $(EXEC_NAME)

clean:
	rm -rf $(SOURCE:.c=.o)
	rm -rf $(EXEC_NAME)

test: $(EXEC_NAME)
	$^

$(SOURCE):
	$(CC) -c $(CFLAGS) $@

$(SOURCE:.c=.o): $(SOURCE)

$(EXEC_NAME): $(SOURCE:.c=.o) 
	$(CC) $(LD_FLAGS) $^ -o $(EXEC_NAME)
