CC = gcc

CFLAGS = -Wall

TARGET = a3chat

all: a3chat

tar: submit

a3chat:
	@$(CC) $(TARGET).c $(CFLAGS) -o $(TARGET)

submit:
	@mkdir submit && cp $(TARGET).c submit && cp Makefile submit && cp ProjectReport.pdf submit && tar -cf submit.tar submit && rm -rf submit

clean:
	@rm -rf a3chat && rm -rf submit*;
