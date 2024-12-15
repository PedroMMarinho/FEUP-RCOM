# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Source files and target
SRC = download.c
TARGET = download

# Default target
all: $(TARGET)

# Compile the source file and directly create the executable
$(TARGET): $(SRC) download.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

# Clean up the generated files
clean:
	rm -f $(TARGET)
