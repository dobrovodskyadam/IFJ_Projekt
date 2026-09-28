# Makefile for a project in course IFJ (Formal Languages and Compilers)
#
# Author: Lukas Sofka (xsofkal00)
# Email: xsofkal00@stud.fit.vutbr.cz
# Faculty of Information Technology, Brno University of Technology

CC = gcc
CXX = g++


# TODO: Add correct compiler flags (e.g. -std=c11, optimization flags, ...)
CFLAGS = -Wall -Wextra
CXXFLAGS = -Wall -Wextra


# Debug flags (uncomment if needed)
DEBUG_CFLAGS = -g
# DEBUG_CFLAGS += -DDEBUG
# DEBUG_CFLAGS += -DDEBUG_MSG


SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)
TARGET = ifj26

.PHONY: build debug clean

build:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

debug:
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)

# End of file
