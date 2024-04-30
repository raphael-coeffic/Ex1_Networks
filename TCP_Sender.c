#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h> 
#include <netinet/tcp.h> 
#include <time.h>


/*
 * @brief The buffer size to store the received message.
 * @note The default buffer size is 1024.
*/
#define BUFFER_SIZE 1024



char *util_generate_random_data(unsigned int size) {
	char *buffer = NULL;
	// Argument check.
	if (size == 0)
		return NULL;
	buffer = (char *)calloc(size, sizeof(char));
	// Error checking.
	if (buffer == NULL)
		return NULL;
	// Randomize the seed of the random number generator.
	srand(time(NULL));
	for (unsigned int i = 0; i < size; i++)
		*(buffer + i) = ((unsigned int)rand() % 256);
	return buffer;
}

/*
 * Sends the content of a file to the server using a socket.
 *
 * @param sock The socket descriptor for the connection to the server.
 * @param content The content of the file to be sent.
 * @param size The size of the content to be sent.
 * @return 0 if the transfer was successful, 1 in case of an error.
 */
int send_file_content(int sock, char* content, unsigned int size) {
    unsigned int bytes_sent_total = 0;
    unsigned int offset = 0; // To track our progress in sending the data
    
    // Loop to send the content in multiple parts if necessary
    while (offset < size) {
        int to_send = size - offset > BUFFER_SIZE ? BUFFER_SIZE : size - offset; // Calculate the amount to send
        int bytes_sent = send(sock, content + offset, to_send, 0); // Send a piece of the content
        if (bytes_sent < 0) {
            perror("Error sending file data");
            close(sock); // Close the socket in case of error
            return 1;
        }
        offset += bytes_sent; // Update the offset with the amount of data sent
        bytes_sent_total += bytes_sent;
    }

    // Sending an end-of-file indicator if necessary
    // To be adapted according to the protocol on the server side
    char eof_indicator = -1; 
    if (send(sock, &eof_indicator, sizeof(eof_indicator), 0) < 0) {
        perror("Error sending EOF data");
        close(sock); // Close the socket in case of error
        return 1;
    }

    printf("The file content was successfully transferred to the server. Total bytes sent: %u\n", bytes_sent_total);

    return 0;
}

/*
 * @brief checking arguments from user function.
 * @param argc - the amount of arguments from the user.
 * @param argv - the array that stores the arguments.
 * @return 0 if the arguments are correct, 1 otherwise.
*/
int check_args(int argc, char *argv[]) {
	
	int error_flag = 0;

	if (argc != 7)
        error_flag = 1;
	if (strncmp(argv[1], "-ip", 3) != 0)
        error_flag = 1;
	if (strncmp(argv[3], "-p", 2) != 0)
        error_flag = 1;
	if (strncmp(argv[5], "-algo", 5) != 0)
        error_flag = 1;
	if (strncmp(argv[6], "reno", 4) != 0 && strncmp(argv[6], "cubic", 5) != 0)
        error_flag = 1;
	
	if (error_flag) {
		fprintf(stderr,"Usage: ./TCP_Sender -ip [IP] -p [PORT] -algo [reno/cubic]\n");
		exit(1);
	}

	return 0;
}

/*
 * @brief TCP Client main function.
 * @param argc - the amount of arguments from the user.
 * @param argv - the array that stores the arguments.
 * @return 0 if the client runs successfully, 1 otherwise.
*/
int main(int argc, char *argv[]) {

	check_args(argc, argv);
	char* server_ip = argv[2];
	unsigned int server_port = (unsigned int)atoi(argv[4]);

	/* ----- setp 1: Read the created file. ----- */

	// Getting the file path from the user.
	unsigned int dataSize = 2 * 1024 * 1024; // size that we want (2MB)
    char *file_content = util_generate_random_data(dataSize);
    if (file_content == NULL) {
        fprintf(stderr, "Failed to generate random data\n");
        return 1;
    }
	
	/* ----- setp 2: Create a TCP socket between the Sender and the Receiver. ----- */

    // The variable to store the socket file descriptor.
    int sock = -1;

    // The variable to store the server's address.
    struct sockaddr_in server;

    // Reset the server structure to zeros.
    memset(&server, 0, sizeof(server));

    // Try to create a TCP socket (IPv4, stream-based, default protocol).
    sock = socket(AF_INET, SOCK_STREAM, 0);

    // If the socket creation failed, print an error message and return 1.
    if (sock == -1)
    {
        perror("Error in creating TCP socket\n");
        return 1;
    }

	// Set the congestion control algorithm to Reno or Cubic.
    if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, argv[6], strlen(argv[6])) < 0) {
        perror("Could not set TCP congestion control to reno or cubic\n");
        close(sock);
        return 1;
    }

    // Convert the server's address from text to binary form and store it in the server structure.
    // This should not fail if the address is valid (e.g. "127.0.0.1").
    if (inet_pton(AF_INET, server_ip, &server.sin_addr) <= 0)
    {
        perror("Error in converting the server's address from text to binary\n");
        close(sock);
        return 1;
    }

    // Set the server's address family to AF_INET (IPv4).
    server.sin_family = AF_INET;

    // Set the server's port to the defined port. Note that the port must be in network byte order,
    // so we first convert it to network byte order using the htons function.
    server.sin_port = htons(server_port);

    fprintf(stdout, "Connecting to %s:%i...\n", server_ip, server_port);

    // Try to connect to the server using the socket and the server structure.
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Error when connecting to the server\n");
        close(sock);
        return 1;
    }

    fprintf(stdout, "Successfully connected to the server!\n");

	/* ----- setp 3: Sending file to the server. ----- */

	printf("Sending generated data to the server...\n");
    if (send_file_content(sock, file_content, BUFFER_SIZE) != 0) {
        free(file_content); // free if error
        close(sock); //close if error
        fprintf(stderr, "Failed to send generated data to the server\n");
        return 1;
    }

	/* ----- Step 4: User decision: Send the file again? ----- */

	char ch = '\0';
	
	while(1) {
		fprintf(stdout, "Send the file again?[Y/N].\n");
		ch = getc(stdin);
		if (ch == 'Y' || ch == 'y') {
			send_file_content(sock, file_content, BUFFER_SIZE);
			continue;
		}
		if (ch == 'N' || ch == 'n') {
			break;
		}
		printf("Please enter 'Y' or 'N'\n");
	}

	/* ----- Step 5: Send an exit message to the receiver. ----- */

	if (send(sock, "EXIT", 4, 0) < 0) {
		perror("Error send exit message.\n");
        close(sock);
		return 1;
	}

	/* ----- Step 6: Close the TCP connection. ----- */

    close(sock);
	free(file_content);
    fprintf(stdout, "Connection closed!\n");


	/* ----- Step 7: Exit. ----- */	

	fprintf(stdout, "TCP_Sender program ran successfully.\nExiting TCP_Sender program...\n");

    return 0;
}
