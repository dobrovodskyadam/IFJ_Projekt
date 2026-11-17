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
DEBUG_CFLAGS = -g -O0
DEBUG_CFLAGS += -DDEBUG_MSG
# DEBUG_CFLAGS += -DDEBUG


SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)
HEADER_FILES = $(wildcard $(SRC_DIR)/*.h)
TARGET = ifj26


TEST_DIR = tests
TEST_C_SRCS = $(filter-out $(SRC_DIR)/main.c, $(SRCS))
TEST_OBJS = $(TEST_C_SRCS:.c=.o)
TEST_SRC = $(wildcard $(TEST_DIR)/*.cpp)

.PHONY: build debug clean test format test-on-merlin

build:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

debug:
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -o $(TARGET) $(SRCS)

format:
	clang-format -i $(TEST_SRC) $(SRCS) $(HEADER_FILES)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_OBJS) $(TEST_SRC)
	$(CXX) $(CXXFLAGS) $(TEST_OBJS) $(TEST_SRC) -lgtest -lgtest_main -lpthread -o run_tests
	./run_tests

clean:
	rm -f $(TARGET) run_tests $(SRC_DIR)/*.o

test-on-merlin:
	./tools/test_on_merlin.sh

# End of file
