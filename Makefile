# Makefile for project in course IFJ (Formal Languages and Compilers)
# Brief:
# Last change: 22/09/2026
# 
# Author: Lukas Sofka, xsofkal00
# Email: xsofkal00@stud.fit.vutbr.cz
# Faculty of Information Technology, Brno University of Technology

CC = gcc
CXX = g++

CFLAGS = -Wall -Wextra
CXXFLAGS = -Wall -Wextra

SRC_DIR = src/
SRCS = 	$(wildcard $(SRC_DIR)*.c)
TARGET = ifj26

.PHONY: build clean 

build:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
	
# End of file