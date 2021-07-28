ALL = mailman client middleman

DIR = ./src

SRCS = ./src/mailman.c ./src/client.c ./src/middleman,c
OBJS = $(SRCS:.c=.o)
OUTPUTS = $(SRCS:.c=)
INCS = 

#Use the gcc compiler for compiling and linking
CC = gcc
CFLAGS = -Wno-deprecated

all: $(ALL)

clean:
	rm -f ./src/*.o *~ $(ALL)

r: clean all

kill:
	killall $(ALL)

$(DIR)/%.o: $(DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

mailman: $(DIR)/mailman.o $(INCS)
	$(CC) $(CFLAGS) -o $@ $^

client: $(DIR)/client.o $(INCS)
	$(CC) $(CFLAGS) -o $@ $^

middleman: $(DIR)/middleman.o $(INCS)
	$(CC) $(CFLAGS) -o $@ $^
