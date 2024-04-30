#include <stdio.h> 
#include <arpa/inet.h> 
#include <sys/socket.h> 
#include <unistd.h> 
#include <string.h> 
#include <time.h> 
#include <stdlib.h>   
#include <netinet/tcp.h>

/*
 * @brief The maximum number of clients that the server can handle.
 * @note The default maximum number of clients is 1.
*/
#define MAX_CLIENTS 1

/*
 * @brief The buffer size to store the received message.
 * @note The default buffer size is 1024.
*/
#define BUFFER_SIZE 1024

/*
 * @brief The buffer size to store the log.
 * @note The default buffer size log is 2048.
*/
#define BUFFER_SIZE_LOG 2048

/*
 * @brief checking arguments from user function.
 * @param argc - the amount of arguments from the user.
 * @param argv - the array that stores the arguments.
 * @return 0 if the arguments are correct, 1 otherwise.
*/
int check_args(int argc, char *argv[]) {
	
	int error_flag = 0;

	if (argc != 5)
        error_flag = 1;
	if (strncmp(argv[1], "-p", 2) != 0)
        error_flag = 1;
	if (strncmp(argv[3], "-algo", 5) != 0)
        error_flag = 1;
	if (strncmp(argv[4], "reno", 4) != 0 && strncmp(argv[4], "cubic", 5) != 0)
        error_flag = 1;
	
	if (error_flag) {
		fprintf(stderr,"Usage: ./TCP_Receiver -p [PORT] -algo [reno/cubic]\n");
		exit(1);
	}
	
	return 0;
}


/*
 * @brief TCP Server main function.
 * @param argc - the amount of arguments from the user.
 * @param argv - the array that stores the arguments.
 * @return 0 if the server runs successfully, 1 otherwise.
*/
int main(int argc, char *argv[]) {

	check_args(argc, argv);
	unsigned int server_port = (unsigned int)atoi(argv[2]);

	/* ---------- Step 1: Create a TCP connection between the Receiver and the Sender. ---------- */

    // The variable to store the socket file descriptor.
    int sock = -1;

    // The variable to store the server's address.
    struct sockaddr_in server;

    // The variable to store the client's address.
    struct sockaddr_in client;

    // Stores the client's structure length.
    socklen_t client_len = sizeof(client);

    // The variable to store the socket option for reusing the server's address.
    //int opt = 1;

    // Reset the server and client structures to zeros.
    memset(&server, 0, sizeof(server));
    memset(&client, 0, sizeof(client));

    // Try to create a TCP socket (IPv4, stream-based, default protocol).
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == -1)
    {
        perror("Error in creating TCP socket\n");
        return 1;
    }

	// Set the congestion control algorithm to Reno or Cubic.
    if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, argv[4], strlen(argv[4])) < 0) {
        perror("Could not set TCP congestion control to reno or cubic\n");
        close(sock);
        return 1;
    }

    // Set the server's address to "0.0.0.0" (all IP addresses on the local machine).
    server.sin_addr.s_addr = INADDR_ANY;

    // Set the server's address ssfamily to AF_INET (IPv4).
    server.sin_family = AF_INET;

    // Set the server's port to the specified port. Note that the port must be in network byte order.
    server.sin_port = htons(server_port);

    // Try to bind the socket to the server's address and port.
    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Error on binding\n");
        close(sock);
        return 1;
    }

	
    // Try to listen for incoming connections.
    if (listen(sock, MAX_CLIENTS) < 0)
    {
        perror("Error on listening\n");
        close(sock);
        return 1;
    }

    fprintf(stdout, "Listening for incoming connections on port %i...\n", server_port);

	/* ---------- Step 2: Get a connection from the sender. ---------- */

    // Try to accept a new client connection.
    int client_sock = accept(sock, (struct sockaddr *)&client, &client_len);

    // If the accept call failed, print an error message and return 1.
    if (client_sock < 0)
    {
        perror("Error in accepting a new client connection.\n");
        close(sock);
        return 1;
    }

	// Print a message to the standard output to indicate that a new client has connected.
    fprintf(stdout, "Client %s:%d connected\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
	
	
	/* The server's main loop. */

	char buffer[BUFFER_SIZE]; // Create a buffer to store the received message.
	char buf_log[BUFFER_SIZE_LOG];
	char buf_temp[BUFFER_SIZE_LOG];
	memset(buf_log, 0, BUFFER_SIZE_LOG);
	memset(buf_temp, 0, BUFFER_SIZE_LOG);
	int bytes_received = 0;
	clock_t t = 0;
	double time_taken = 0; // Track the amnount of time taken to get a file.
    long data_transferred = 0; // Track the amount of data transferred.
    double speed_mbps = 0; // Speed in Megabytes per milliseconds.
	double total_time_taken = 0;
	double total_speed_mbps = 0;
	int run_times = 0;

    while (1)
    {
		/* ---------- Step 3: Receive the file, measure the time it took and save it. ---------- */

		fprintf(stdout, "Getting file from sender.\n");

		// Adding the amount of run times.
		run_times++;

		// Initialize variables
		memset(buffer, 0, BUFFER_SIZE);
		time_taken = 0;
		speed_mbps = 0;
		
		// Start clock
		t = clock(); 

		// If the client decided tp resend the file then remember the data that was received from step 4.
		data_transferred = bytes_received;

        // Receive a message from the client and store it in the buffer.
		while ((bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0)) > 0) {
			data_transferred += bytes_received;
		    if (buffer[0] == -1) 
				break;
		}
		
		// Stop clock and calculate time taken in ms
		t = clock() - t; 
		time_taken = (((double)t)*1000/CLOCKS_PER_SEC); 

		// Calculate speed in MB/s
    	speed_mbps = (data_transferred / 1024.0/ 1024.0) / (time_taken / 1000.0);
        // If the message receiving failed, print an error message and return 1.
        if (bytes_received < 0)
        {
            perror("recv failed");
            close(client_sock);
            close(sock);
            return 1;
        }

        // If the amount of received bytes is 0, the client has disconnected.
        else if (bytes_received == 0)
        {
            fprintf(stdout, "Client %s:%d disconnected\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
            close(client_sock);
            break;
        }
		fprintf(stdout, "File transfer completed.\n");

		// Get stats for this run
        sprintf(buf_temp, "- Run #%i Data: Time=%.2fms; Speed=%.2fMB/s\n", run_times, time_taken, speed_mbps);
		memcpy(buf_log + strlen(buf_log), buf_temp, strlen(buf_temp));
		total_time_taken += time_taken;
		total_speed_mbps += speed_mbps;

		/* ---------- Step 4: Wait for Sender response: ---------- */

		fprintf(stdout, "Waiting for response from client...\n");
		if ((bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0)) > 0) {
			// If sender sent EXIT then break the loop and go to step 5.
			if (strncmp(buffer, "EXIT", 4) == 0) {
				fprintf(stdout, "Server got EXIT message from client\n");
				break;
			}
			// If sender sent the file again then go back to step 3.
			else
				continue;
		}
		if (bytes_received < 0)
		{
		    perror("recv failed when waiting on exit message\n");
		    close(client_sock);
		    close(sock);
		    return 1;
		}  
    }

	
    close(client_sock);
	close(sock);

    fprintf(stdout, "Client %s:%d disconnected and sent exit message.\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

	/* ---------- Step 5 and 6: Print out the times (in milliseconds), and the average bandwidth for each time the file was received. ---------- */

	/* log format to stdout */
	fprintf(stdout, "---------------------------    -------\n");
	fprintf(stdout, "-        * Statistics *          -\n");
	fprintf(stdout, "%s", buf_log);
	fprintf(stdout, "-                                 \n");
	fprintf(stdout, "- Average time: %.2fms            \n", total_time_taken / run_times);
	fprintf(stdout, "- Average bandwidth: %.2fMB/s     \n", total_speed_mbps / run_times);
	fprintf(stdout, "----------------------------------\n");

    fprintf(stdout, "Server finished!\n");

	/* ----- Step 7: Exit. ----- */

    return 0;
}
