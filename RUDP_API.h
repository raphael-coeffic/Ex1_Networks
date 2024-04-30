/**
 * 337614747
 * 315173633
*/


#ifndef RUDP_API_H
#define RUDP_API_H


//use bitfields for flags
typedef struct FLAGS {
  unsigned int SYN : 1;
  unsigned int ACK : 1;
  unsigned int DATA : 1;
  unsigned int FIN : 1;
}flags;
//struct of our rudp
typedef struct RUDP {
  flags flags;
  int seq;
  int checksum;
  int size;
  char val[5000];
}rudp;
// creation of a new socket
int rudp_socket();
//rdup sending
int rudp_send(int socket, char *data, int data_length);
//rudp receiving
int rudp_recv(int socket, char **data, int *data_length);
//closing of the connection
int rudp_close(int socket);
//func of the appendix for generate random file
char *util_generate_random_data(unsigned int size);
//necessary for calculate checksum beacause it's a reliable UDP
int calculate_checksum(rudp *packet);

#endif