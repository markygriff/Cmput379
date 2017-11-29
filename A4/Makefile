CC = gcc

CFLAGS = -Wall -lm

TARGET = a4vmsim

all: a4vmsim

tar: submit

a3chat:
	@$(CC) $(TARGET).c $(CFLAGS) -o $(TARGET)

submit:
	@mkdir submit && cp $(TARGET).c submit && cp Makefile submit && cp ProjectReport.pdf submit && tar -cf submit.tar submit && rm -rf submit

clean:
	@rm -rf a4vmsim && rm -rf submit*;
