
CC = gcc

BUILD_DIR := build
OBJ_DIR := obj

EXECUTABLES = logappend logread
 
SOURCES := $(wildcard *.c)
OBJECTS := $(patsubst %, $(OBJ_DIR)/%.o, $(filter-out $(EXECUTABLES), $(patsubst %.c, %, $(SOURCES))))

CFLAGS := -I /usr/local/opt/openssl@1.1/include
LDFLAGS := -lssl -lcrypto -L /usr/local/opt/openssl@1.1/lib

all: $(EXECUTABLES)

$(EXECUTABLES) : $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -g $@.c -o $(BUILD_DIR)/$@ $(OBJECTS)

$(OBJECTS): $(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -g -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/* $(BUILD_DIR)/*
