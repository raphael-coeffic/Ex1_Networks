CC=gcc
CFLAGS=-Wall -g
LDFLAGS=

all: TCP_Receiver TCP_Sender RUDP_Receiver RUDP_Sender

TCP_Sender: TCP_Sender.o
	$(CC) $(LDFLAGS) -o TCP_Sender TCP_Sender.o

TCP_Receiver: TCP_Receiver.o
	$(CC) $(LDFLAGS) -o TCP_Receiver TCP_Receiver.o

TCP_Sender.o: TCP_Sender.c
	$(CC) $(CFLAGS) -c TCP_Sender.c

TCP_Receiver.o: TCP_Receiver.c
	$(CC) $(CFLAGS) -c TCP_Receiver.c

RUDP_Sender: RUDP_Sender.o RUDP_API.o
	$(CC) $(LDFLAGS) -o RUDP_Sender RUDP_Sender.o RUDP_API.o

RUDP_Receiver: RUDP_Receiver.o RUDP_API.o
	$(CC) $(LDFLAGS) -o RUDP_Receiver RUDP_Receiver.o RUDP_API.o

RUDP_Sender.o: RUDP_Sender.c RUDP_API.h
	$(CC) $(CFLAGS) -c RUDP_Sender.c

RUDP_Receiver.o: RUDP_Receiver.c RUDP_API.h
	$(CC) $(CFLAGS) -c RUDP_Receiver.c

RUDP_API.o: RUDP_API.c RUDP_API.h
	$(CC) $(CFLAGS) -c RUDP_API.c

clean:
	rm -f *.o TCP_Receiver TCP_Sender RUDP_Receiver RUDP_Sender