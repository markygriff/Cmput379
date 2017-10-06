CC = gcc

CFLAGS = -Wall

TARGET = a2rchat

all: a2rchat

tar: submit

a1shell:
	@$(CC) $(TARGET).c $(CFLAGS) -o $(TARGET)

submit:
	@mkdir submit && cp $(TARGET).c submit && cp Makefile submit && cp ProjectReport.pdf submit && tar -cf submit.tar submit && rm -rf submit

clean:
	@rm -rf a2chat && rm -rf submit*
