CC = gcc

CFLAGS = -Wall

TARGET = a1shell

all: a1shell

tar: submit

a1shell:
	@$(CC) $(TARGET).c $(CFLAGS) -o $(TARGET)

submit:
	@mkdir submit && cp $(TARGET).c submit && cp Makefile submit && tar -cf submit.tar submit && rm -rf submit

clean:
	@rm -rf a1shell && rm -rf submit*
