#include "RUDP_API.h"
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

// creation of a new socket
//in use both in sender and receiver
int rudp_socket() 
{
  int send_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (send_socket == -1) //check error
  {            
    perror("failed create socket\n");
    return -1;
  }

  return send_socket;
}
// this func send with rudp
//in use in sender
int rudp_send(int socket, char *data, int data_length) 
{
  clock_t c = clock();
  int val = 0;
  int index = 0;
  // how many packets do we send and what is se size of the last one 
  int num_of_packets = data_length / 5000;
  int last_one = data_length % 5000;
  //allocation of the packet
  rudp *p = malloc(sizeof(rudp));

  // main loop to send all packets except the potential final partial packet  int index = 0;
  while (index < num_of_packets) 
  {
    memset(p, 0, sizeof(rudp));
    p->seq = index;
    p->flags.DATA = 1;
    if (index == num_of_packets - 1 && last_one == 0) p->flags.FIN = 1;
    // enter the value in the packet
    memcpy(p->val, data + index * 5000, 5000);
    p->size = 5000;
    p->checksum = calculate_checksum(p);
    do {  // here we wait for the ACK
      int send = sendto(socket, p, sizeof(rudp), 0, NULL, 0);
      if (send == -1) 
      {
        perror("Error on sending\n");
        return -1;
      }
      rudp *reply = malloc(sizeof(rudp));
      while ((double)(clock() - c) / CLOCKS_PER_SEC < 1) 
      {
        int recv = recvfrom(socket, reply, sizeof(rudp) - 1, 0, NULL, 0);
        if (recv == -1) 
        {
          val = recv;
          break;
        }
        if (reply->seq == index && reply->flags.ACK == 1) 
        {//we receive
          val = 1;
          break;
        }
      }
      free(reply);
    } while (val <= 0);
    //init
    c = clock();
    val = 0;
    index++;
  }
  // send the last packet if it contains less than 5000 bytes
  if (last_one > 0) 
  {
    memset(p, 0, sizeof(rudp));
    p->seq = num_of_packets;
    p->flags.DATA = 1;
    p->flags.FIN = 1;  // this is definitely the final packet
    memcpy(p->val, data + num_of_packets * 5000,
           last_one);
    p->size = last_one;
    p->checksum = calculate_checksum(p);
    do {  // sending the packet and waiting for an ACK
      int send = sendto(socket, p, sizeof(rudp), 0, NULL, 0);
      if (send == -1) 
      {
        perror("sendto failed\n");
        free(p);
        return -1;
      }
      rudp *reply = malloc(sizeof(rudp));
      while ((double)(clock() - c) / CLOCKS_PER_SEC < 1) 
      {
        int recv = recvfrom(socket, reply, sizeof(rudp) - 1, 0, NULL, 0);
        if (recv == -1) 
        {
          val = recv;
          break;
        }
        else if (reply->seq == num_of_packets && reply->flags.ACK == 1) 
        {//we receive
          val = 1;
          break;// break if the correct ACK is received
        }
      }
      free(reply);
    } while (val <= 0);
    free(p);
  }
  return 1;
}
int sequence = 0;
// reception of data
//in use in receiver
int rudp_recv(int socket, char **data, int *data_size) 
{
  //allocation of the packet
  rudp *p = malloc(sizeof(rudp));
  memset(p, 0, sizeof(rudp));

  int recv = recvfrom(socket, p, sizeof(rudp) - 1, 0, NULL, 0);
  if (recv == -1) 
  {
    perror("recv failed\n");
    free(p);
    return -1;
  }
  if (p->checksum != calculate_checksum(p)) //compare the checksum
  {
    free(p);
    return -1;
  }
  //allocation for ack packet
  rudp *ack = malloc(sizeof(rudp));
  memset(ack, 0, sizeof(rudp));
  ack->flags.ACK = 1;
  //update the flags
  if (p->flags.FIN == 1) ack->flags.FIN = 1;
  if (p->flags.SYN == 1) ack->flags.SYN = 1;
  if (p->flags.DATA == 1) ack->flags.DATA = 1;
  ack->seq = p->seq;
  ack->checksum = calculate_checksum(ack);
  int send = sendto(socket, ack, sizeof(rudp), 0, NULL, 0);
  if (send == -1) 
  {
    perror("Error on sending\n");
    free(ack);
    return 0;
  }
  free(ack);
  //connection
  if (p->flags.SYN == 1) 
  {
    free(p);
    return 0;
  }
  if (p->seq == sequence) 
  {
    if (p->seq == 0 && p->flags.DATA == 1) 
    {
      struct timeval timeout;
      timeout.tv_sec = 10;
      timeout.tv_usec = 0;

      if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) 
      {
        return -1;
      }
    }
    //for the last packet that we receive
    if (p->flags.FIN == 1 && p->flags.DATA == 1) 
    {
      *data = malloc(p->size);  
      memcpy(*data, p->val, p->size);
      *data_size = p->size;
      free(p);
      sequence = 0;
      struct timeval timeout;
      timeout.tv_sec = 10000000;
      timeout.tv_usec = 0;
      if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) return -1;
      return 2;
    }
    if (p->flags.DATA == 1) 
    {  
      *data = malloc(p->size);
      //copy the data
      memcpy(*data, p->val, p->size);
      *data_size = p->size;
      free(p);
      sequence++;
      return 1;
    }
  } else if (p->flags.DATA == 1) 
  {
    free(p);
    return 0;
  }
  if (p->flags.FIN == 1) 
  {  // close request
    free(p);
    // if after 3 seconds the sender closed it's finish
    printf("Close the connection\n");
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) <0) 
    {
      perror("Error\n");
      return -1;
    }
 

    p = malloc(sizeof(rudp));
    time_t FIN = time(NULL);
    //closing
    while ((double)(time(NULL) - FIN) < 1) 
    {
      memset(p, 0, sizeof(rudp));
      recvfrom(socket, p, sizeof(rudp) - 1, 0, NULL, 0);
      if (p->flags.FIN == 1) {
        rudp *ack = malloc(sizeof(rudp));
        memset(ack, 0, sizeof(rudp));
        ack->flags.ACK = 1;
        if (p->flags.FIN == 1) ack->flags.FIN = 1;
        if (p->flags.SYN == 1) ack->flags.SYN = 1;;
        if (p->flags.DATA == 1) ack->flags.DATA = 1;
        ack->seq = p->seq;
        ack->checksum = calculate_checksum(ack);
        int send = sendto(socket, ack, sizeof(rudp), 0, NULL, 0);
        if (send == -1) 
        {
          perror("Error on sending\n");
          free(ack);
          return 0;
        }
        free(ack);
        FIN = time(NULL);
      }
    }
    free(p);
    close(socket);
    return -2;
  }
  free(p);
  return 0;
}

// close the connection
int rudp_close(int socket) 
{
  //allocation of the packet and init
  rudp *p = malloc(sizeof(rudp));
  memset(p, 0, sizeof(rudp));
  p->flags.FIN = 1;
  p->seq = -1;
  p->checksum = calculate_checksum(p);
  int val = 0;
  clock_t c = clock();
  do {
    int send = sendto(socket, p, sizeof(rudp), 0, NULL, 0);
    if (send == -1) 
    {
      perror("Error on sending\n");
      free(p);
      return -1;
    }
    rudp *reply = malloc(sizeof(rudp));
    while ((double)(clock() - c) / CLOCKS_PER_SEC < 1) 
    {
      int recv = recvfrom(socket, reply, sizeof(rudp) - 1, 0, NULL, 0);
      if (recv == -1) 
      {
        val = recv;
        break;
      }
      else if (reply->seq == -1 && reply->flags.ACK == 1) 
      {
        val = 1;
        break;
      }
    }
    free(reply);
  } while (val <= 0);
  close(socket);
  free(p);
  return 1;
}

//in use in sender because he generate the file that he generate the file
char *util_generate_random_data(unsigned int size) {
  char *buffer = NULL;
  // Argument check.
  if (size == 0) return NULL;
  buffer = (char *)calloc(size, sizeof(char));
  // Error checking.
  if (buffer == NULL) return NULL;
  // Randomize the seed of the random number generator.
  srand(time(NULL));
  for (unsigned int i = 0; i < size; i++)
    *(buffer + i) = ((unsigned int)rand() % 255) + 1;
  return buffer;
}

int calculate_checksum(rudp *packet) 
{
  if (!packet)
  {
    perror("Can't find the packet for checksum\n");
    return -1;
  }
  int checksum = 0;
  //compute the checksum
  for (int i = 0; i < 10 && i < 5000; i++) checksum += packet->val[i];
  return checksum;
}