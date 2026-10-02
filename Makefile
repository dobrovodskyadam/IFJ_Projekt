# Makefile for a project in course IFJ (Formal Languages and Compilers)
#
# Author: Lukas Sofka (xsofkal00)
# Email: xsofkal00@stud.fit.vutbr.cz
# Faculty of Information Technology, Brno University of Technology

CC = gcc
CXX = g++


# TODO: Add correct compiler flags (e.g. -std=c11, optimization flags, ...)
CFLAGS = -Wall -Wextra
CXXFLAGS = -Wall -Wextra -I$(SRC_DIR)


# Debug flags (uncomment if needed)
DEBUG_CFLAGS = -g
# DEBUG_CFLAGS += -DDEBUG
# DEBUG_CFLAGS += -DDEBUG_MSG


SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)
TARGET = ifj26

TEST_DIR = tests
TEST_C_SRCS = $(filter-out $(SRC_DIR)/main.c, $(SRCS))
TEST_OBJS = $(TEST_C_SRCS:.c=.o)
TEST_SRC = $(wildcard $(TEST_DIR)/*.cpp)

.PHONY: build debug clean test

build:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

debug:
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -o $(TARGET) $(SRCS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_OBJS) $(TEST_SRC)
	$(CXX) $(CXXFLAGS) $(TEST_OBJS) $(TEST_SRC) -lgtest -lgtest_main -lpthread -o run_tests
	./run_tests

clean:
	rm -f $(TARGET) run_tests $(SRC_DIR)/*.o

# End of file
