CC = gcc

CFLAGS = -Wall

TARGET = a1shell

all: a1shell

tar: submit

a1shell:
	@$(CC) $(TARGET).c $(CFLAGS) -o $(TARGET).o

submit:
	mkdir submit && cp $(TARGET).c submit && cp Makefile submit && tar -cf submit.tar submit

clean:
	@rm -rf *.o && rm -rf submit*
