DEL=rm
CC=gcc
#CC=/work/compiler/gcc-linaro-arm-linux-gnueabihf-4.7-2013.04-20130415_linux/bin/arm-linux-gnueabihf-gcc
#CC=/opt/gcc-linaro-arm-linux-gnueabihf-4.7-2013.04-20130415_linux/bin/arm-linux-gnueabihf-gcc

CFLAGS+=-static
CFLAGS+=-lpthread -pthread
#CFLAGS+=-fopenmp
#CFLAGS+=-lm
#CFLAGS+=-lncurses
#CFLAGS+=-L/location/of/ncurses -lGL -lGLU -lglut -lncurses
#CFLAGS+=-std=c99
CFLAGS+=-g
#CFLAGS+=-Wall
#CFLAGS+=-MM

TARGET=main
INCDIR=./inc
SRCDIR=./src
OBJDIR=./src/obj
SRCS=$(wildcard $(SRCDIR)/*.c)
OBJS=$(patsubst %.c,$(OBJDIR)/%.o,$(notdir $(SRCS)))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJS): $(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) -I $(INCDIR) -c $(CFLAGS) $< -o $@

clean:
	$(DEL) -rf $(OBJDIR)/*.o
	$(DEL) -rf $(TARGET)
