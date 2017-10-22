CC = gcc

CFLAGS = -Wall

TARGET = a2chat

all: a2chat

tar: submit

a2chat:
	@$(CC) $(TARGET).c $(CFLAGS) -o $(TARGET)

submit:
	@mkdir submit && cp $(TARGET).c submit && cp Makefile submit && cp ProjectReport.pdf submit && tar -cf submit.tar submit && rm -rf submit

clean:
	@rm -rf a2chat && rm -rf submit*; find . -maxdepth 1 -type p -delete
