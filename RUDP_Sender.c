#include "RUDP_API.h"
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(int argc, char *argv[]) {
  // input
  char* server_ip;
  int server_port;
   if (argc != 5 ||  (strcmp(argv[1], "-ip") != 0 && strcmp(argv[3], "-p") != 0)) {
    fprintf(stderr, "Usage: ./RUDP_Sender -ip <IP> -p <PORT>\n");
    return 0;
  }
  server_ip = argv[2];
  server_port = atoi(argv[4]);

  /* ----- setp 1: Read the created file. ----- */
  // create the file withe the func of appendix
  char *file_content = util_generate_random_data(2097152);

  /* ----- setp 2: Create a UDP socket between the Sender and the Receiver. ----- */
  // new socket
  int socket = rudp_socket();
  if (socket == -1) {
    return -1;
  }
  
  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  int bool = 0;

  if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) <0) {
    perror("error\n");
    return -1;
  }
  
  // The variable to store the server's address.
  struct sockaddr_in adrr;
  // Reset the server structure to zeros.
  memset(&adrr, 0, sizeof(adrr));
  // Set the server's address family to AF_INET (IPv4).
  adrr.sin_family = AF_INET;

  // Set the server's port to the defined port. Note that the port must be in network byte order,
  // so we first convert it to network byte order using the htons function.
  adrr.sin_port = htons(server_port);
  
  // Convert the server's address from text to binary form and store it in the server structure.
  // This should not fail if the address is valid (e.g. "127.0.0.1").
  if (inet_pton(AF_INET, (const char *)server_ip, &adrr.sin_addr) <= 0)
  {
      perror("Error in converting the server's address from text to binary\n");
      rudp_close(socket);
      return 1;
  }

  fprintf(stdout, "Connecting to %s:%i...\n", server_ip, server_port);
  if (connect(socket, (struct sockaddr *)&adrr, sizeof(adrr)) == -1) {
    perror("Error when connecting to the server\n");
    rudp_close(socket);
    return 1;
  }

  // SYN
  rudp *p = malloc(sizeof(rudp));
  memset(p, 0, sizeof(rudp));
  p->flags.SYN = 1;
  int tries = 0;
  rudp *receive = NULL;
  while (tries < 3) {  //we try 3 times
    int send = sendto(socket, p, sizeof(rudp), 0, NULL, 0);
    if (send == -1) {
      perror("Error on sending\n");
      free(p);
      return -1;
    }
    clock_t start = clock();
    while ((double)(clock() - start) / CLOCKS_PER_SEC < 1){
      // SYN_ACK
      receive = malloc(sizeof(rudp));
      memset(receive, 0, sizeof(rudp));
      int res = recvfrom(socket, receive, sizeof(rudp), 0, NULL, 0);
      if (res == -1) {
        perror("Error on recv\n");
        free(p);
        free(receive);
        return -1;
      }
      if (receive->flags.SYN && receive->flags.ACK) {
        fprintf(stdout, "You are connected\n");
        free(p);
        free(receive);
        bool = 1;
        break;
      } else {
        perror("Error on reception\n");
        free(p);
        free(receive);
        return -1;
      }
    }
    if (bool == 1) break;
    tries++;
  }
  /* ----- setp 3: Send the file via the RUDP protocol ----- */
  printf("Sending generated data to the server...\n");
  if (rudp_send(socket, file_content, 2097152) < 0) {
    rudp_close(socket);
    fprintf(stderr, "Failed to send generated data to the server\n");
    return 1;
  
  }
  char ch = 'y';    //sending
  while (1){
    
    /* ----- Step 4: User decision: Send the file again? ----- */

    printf("Send the file again?[Y/N].\n ");        //the user choose
    scanf(" %c", &ch);
    if (ch == 'Y' || ch == 'y') 
    {
      printf("Sending generated data to the server...\n");
			if (rudp_send(socket, file_content, 2097152) < 0) {
        perror("Error on sending\n");
        rudp_close(socket);
        return 1;
      }
		}
    if (ch == 'N' || ch == 'n') break;

  }
  /* ----- Step 6: Close the UDP connection. ----- */
  rudp_close(socket);       //end of the connection
  /* ----- Step 7: Exit. ----- */
  fprintf(stdout, "RUDP_Sender program ran successfully.\nExiting RUDP_Sender program...\n");
  free(file_content);
  return 0;
}
