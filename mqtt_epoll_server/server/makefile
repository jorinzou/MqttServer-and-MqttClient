CC=gcc

OBJ=server
all:$(OBJ)

HERE=$(shell pwd)
incs=-I$(HERE)

SRC=$(HERE)/src/
files:=$(foreach dir,$(SRC),$(wildcard $(SRC)/*.c))

OBJS_C := $(patsubst %.c,%.o,$(files))

CFLAGS=-O0 -g 
CFLAGS+=$(incs) 

LDFLAGS:=-lpthread 
LDFLAGS+=-lm
LDFLAGS+=-ldl

$(OBJ):$(OBJS_C)
	$(CC)  $(LDFLAGS) -o $@ $^ 
			
$(OBJS_C):%.o:%.c
	$(CC) $(CFLAGS)  -c $< -o $@

clean:
	rm -rf $(HERE)/*.o
	rm -rf $(HERE)/$(OBJ)

