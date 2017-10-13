CC = gcc

CFLAGS = -Wall

TARGET = a2rchat

all: a2rchat

tar: submit

a2rchat:
	@$(CC) $(TARGET).c $(CFLAGS) -o $(TARGET)

submit:
	@mkdir submit && cp $(TARGET).c submit && cp Makefile submit && cp a2answers.pdf submit && cp readme submit && tar -cf submit.tar submit && rm -rf submit

clean:
	@rm -rf a2rchat && rm -rf submit*; find . -maxdepth 1 -type p -delete
