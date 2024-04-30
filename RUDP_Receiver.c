#include "RUDP_API.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char* argv[]) 
{
  // input and check the arguements
  if (argc != 3 || strcmp(argv[1], "-p") != 0) 
  {
    fprintf(stderr, "Usage: ./RUDP_Receiver -p <port>\n");
  }
  int port = atoi(argv[2]);
  printf("Receive on port %d\n", port);

  /* ---------- Step 1: Create a UDP connection between the Receiver and the Sender. ---------- */
  int socket= rudp_socket();
  if (socket== -1) 
  {
    perror("Error in creating RUDP socket\n");
    return 1;
  }

  struct sockaddr_in adrr;
  memset(&adrr, 0, sizeof(adrr));
  // Set the server's address ssfamily to AF_INET (IPv4).
  adrr.sin_family = AF_INET;
  // Set the server's port to the specified port. Note that the port must be in network byte order.
  adrr.sin_port = htons(port);
  adrr.sin_addr.s_addr = htonl(INADDR_ANY);

  // Try to bind the socket to the server's address and port.
  if (bind(socket, (struct sockaddr *)&adrr, sizeof(adrr)) == -1) 
  {
    perror("Error on biding\n");
    rudp_close(socket);
    return -1;
  }

  // receive SYN here
  struct sockaddr_in dest_adrr;
  socklen_t dest_adrr_size = sizeof(dest_adrr);
  memset((char *)&dest_adrr, 0, sizeof(dest_adrr));

  rudp *p = malloc(sizeof(rudp));
  memset(p, 0, sizeof(rudp));
  int receive = recvfrom(socket, p, sizeof(rudp) - 1, 0,(struct sockaddr *)&dest_adrr, &dest_adrr_size);

  if (receive == -1) 
  {
    perror("Error in reception\n");
    free(p);
    return -1;
  }
  /* ---------- Step 2: Get a connection from the sender. ---------- */
  // connection
  if (connect(socket, (struct sockaddr *)&dest_adrr, dest_adrr_size) ==-1)
  {
    perror("Error on connecting\n");
    free(p);
    return -1;
  }

  // SYN_ACK
  if (p->flags.SYN == 1) 
  {
    rudp *response = malloc(sizeof(rudp));
    memset(p, 0, sizeof(rudp));
    response->flags.SYN = 1;
    response->flags.ACK = 1;
    int send = sendto(socket, response, sizeof(rudp), 0, NULL, 0);
    if (send == -1) 
    {
      perror("Error on sending\n");
      free(p);
      free(response);
      return -1;
    }
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) <0) 
    {
      return -1;
    }
    free(p);
    free(response);
  }
  else 
  {
    perror("Can't connect\n");
    return -1;
  }
  printf("Client connected\n");


  /* ---------- Step 3: Receive the file, measure the time it took and save it. ---------- */
  double avr_time = 0;
  double avr_band = 0;
  clock_t start, end;

  char* date_received = NULL;
  int data_length = 0;
  char data[2097152] = {0};  // 2*1,048,576
  int value = 0;
  int run_times = 1;

  start = clock();
  end = clock();

  while (value >= 0) 
  {
    /* ---------- Step 4: Wait for Sender response: ---------- */
    value = rudp_recv(socket, &date_received, &data_length);
    if (value == -1) 
    {
      perror("recv failed");
      rudp_close(socket);
      return 1;
    }
    if (value == 1 && start < end) start = clock();

    if (value == 1) strcat(data, date_received);
    if (value == 2) 
    {
      /* ---------- Step 5 and 6: Print out the times (in milliseconds), and the average bandwidth for each time the file was received. ---------- */
      strcat(data, date_received);
      end = clock();
      double time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;// ms
      avr_time += time;
      double speed = 2 / time;
      avr_band += speed;
      printf("Run %d- Data: Time=%f ms , Speed=%f MB/ms\n", run_times, time, speed);
      memset(data, 0, sizeof(data));
      run_times++;
    }
  }
  avr_time = avr_time / (run_times - 1);
  avr_band = avr_band / (run_times - 1);
  fprintf(stdout, "---------------------------    -------\n");
	fprintf(stdout, "-        * Statistics *          -\n");  
  fprintf(stdout,"\n-Average time: %f ms\n", avr_time);
  fprintf(stdout,"-Average bandwidth: %f MB/ms\n", avr_band);
  fprintf(stdout, "----------------------------------\n");
  /* ----- Step 7: Exit. ----- */
  fprintf(stdout, "Server finished!\n");
  return 0;
}

	