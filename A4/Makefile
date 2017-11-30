CC = gcc
CFLAGS = -Wall
INCLUDES = -I.

TARGET = a4vmsim
SRC = a4vmsim.c pfault.c ptable.c memory.c statistics.c options.c
# OBJS = $(SRC:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) -o $(TARGET)

all: $(TARGET)

tar: submit

submit:
	@mkdir submit && cp $(TARGET).c submit && cp Makefile submit && \
	cp ProjectReport.pdf submit && tar -cf submit.tar submit && rm -rf submit

clean:
	@rm -f a4vmsim && rm -rf submit*;
