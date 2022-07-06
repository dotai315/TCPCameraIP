TARGET = camera

CC = gcc
CFLAGS = -g -Wall 

SRC = src
SRCS = $(wildcard $(SRC)/*.c)

OBJ = obj
OBJS = $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))

INC = inc
INCLUDES = -I$(INC)

BIN_DIR = bin
BIN = $(BIN_DIR)/$(TARGET)

LIBS =-lpthread

all: $(BIN)

$(BIN): $(OBJS) $(OBJ)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $@ `pkg-config --cflags --libs libv4l2`

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBS) -c $< -o $@ `pkg-config --cflags --libs libv4l2`

$(OBJ):
	mkdir -p $@

clean:
	rm -rf $(BIN_DIR)/* $(OBJ)/*

edit:
	vim -O $(SRC)/*.c $(INC)/*.h

dir:
	mkdir inc src obj bin 
	touch $(INC)/main.h $(SRC)/main.c

create-project:
	mkdir $(TARGET) 
	cp ./Makefile ./$(TARGET)/

delete-project:
	rm -rf inc src obj bin
