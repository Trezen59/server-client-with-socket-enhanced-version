/*

Author: Trezen Parmar
E-MAIL: trezen7984891023@gmail.com

DESCRIPTION:

This app is a server-client app which uses sockets mainly for communication.
This is a bidirectional app in which both participants can send and receive
messages simultaneously. It also has a menu based approach to select the
appropriate options.

USE:
1. Compile the server and client app using the command "$make"
2. Run the server "$./server" and then run the client "$./client".
3. Select an option from the Menu shown.
4. Enjoy the chat.
5. Follow the instructions.

*/

#include "libServer.h"


int sendFileToClient(int cliFD)
{
	int ret = -1;
	int fileFD = -1;
    char fileLocation[MAX] = "/home/einfochips/users/trezen/files/server-client-enhanced/ServerFiles/";
    char fileName[MAX] = "toClient.txt";
    struct stat st;

    strcat(fileLocation, fileName);

	/* check if file exists */
	ret = access(fileLocation, F_OK | R_OK);
	if(ret < 0){
		printf("file '%s' not found\n", fileName);
		return -1;
	}
	printf("file is found and can be read..\n\n");

	fileFD = open((char *)fileLocation, O_RDONLY);
	if(fileFD < 0){
		printf("error in opening fd\n");
		return -1;
	}

	/* get file size */
	stat(fileLocation, &st);
	int *size = malloc(sizeof(int));
	*size = st.st_size;

	/* init buffer with file size */
	char *buff = (char *)malloc(*size);
	bzero(buff, *size);

	if (SERVER_DEBUG_PRINTS)
		printf("size of file is %d\n", *size);

	/* send size of file */
	write(cliFD, size, sizeof(int));

	/* store contents of file into buffer */
	int len = read(fileFD, buff, *size);
	buff[len] = '\0';

	if (SERVER_DEBUG_PRINTS)
		printf("lenth read = %d\n", len);
	if (SERVER_DEBUG_PRINTS)
		printf("file contents : %s\n", buff);

	/* send file */
	ret = write(cliFD, buff, *size);
	if(ret < 0){
		printf("write unsuccessful\n");
		return -1;
	} else {
		printf("======== write successful. File sent ========\n");
	}

	bzero(buff, *size);
	free(size);
	return 0;
}

int receiveFileFromClient(int connFD)
{
	int ret = -1;
	int fileFD = -1;
    char fileLocation[MAX] = "/home/einfochips/users/trezen/files/server-client-enhanced/ServerFiles/";
    char fileName[MAX] = "fromClient.txt";

    /* init size to store size of file */
    int *size = malloc(sizeof(int));
    bzero(size, sizeof(int));

    strcat(fileLocation, fileName);

    /* check if file exists at a given location */
    ret = access(fileLocation, F_OK | R_OK);
	if (ret >= 0){
        printf("file already found\n\n");
	}

	/* open and create file if file not available */
	fileFD = open((char *)fileLocation, O_RDWR | O_CREAT);
	if(fileFD < 0){
		printf("error in opening fd\n");
		return -1;
	}

	/* get file size */
	ret = read(connFD, size, sizeof(int));
	if (ret < 0)
		perror("read");
	else {
		if (SERVER_DEBUG_PRINTS)
			printf("read length success\n");
	}

	if (SERVER_DEBUG_PRINTS)
		printf("size recieved is %d\n", *size);

	char *buff = (char *)malloc(*size);
	bzero(buff, *size);

	/* read file into buff */
	ret = read(connFD, buff, *size);
	if(ret < 0){
		printf("read unsuccessful\n");
		return -1;
	}

	if (SERVER_DEBUG_PRINTS)
		printf("buffer value after read is : %s", buff);

	/* write the contents of buff to file */
	int len = write(fileFD, buff, *size);
	if(len < 0){
		printf("write unsuccessful\n");
	} else {
		printf("======== written to file ========\n");
		if (SERVER_DEBUG_PRINTS)
			printf("buffer content after write : %s", buff);
	}
	bzero(buff, *size);
	free(size);
	return 0;
}

void *reading(void *server_data)
{
	int flag = 1;
	int ret = -1;
	char buff[MAX_CHAT_MSG_LEN] = {0};

	/* Infinit Loop */
	while(flag)
	{
		bzero(buff, MAX_CHAT_MSG_LEN);

		/* read data into buff */
		ret = read(((SERVER_DATA *)server_data)->mainClientFD, buff, MAX_CHAT_MSG_LEN);
		if(ret < 0)
		{
			perror("read :");
			break;
		}
		printf("\n\t\t\t %s\n", buff);

		/* compare and check for "bye" to exit chat */
		if(!(strncmp("bye", buff , 3)))
		{
			flag = 0;
			break;
		}
	}
	pthread_exit(NULL);
}

void *writing(void *server_data)
{
	int flag = 1;
	int ret = -1;
	char buff[MAX_CHAT_MSG_LEN] = {0};

	while(flag)
	{
		int i = 0;
		printf("\n");

		bzero(buff, MAX_CHAT_MSG_LEN);

		/* get the message and store it in buffer */
		while (( buff[i++] = getchar() ) != '\n' || i == MAX_CHAT_MSG_LEN)
		buff[i] = '\0';

		/* write the content */
		ret = write(((SERVER_DATA *)server_data)->secondaryClientFD, buff, MAX_CHAT_MSG_LEN);
		if(ret < 0)
		{
			perror("write : ");
			break;
		}

		/* compare msg for "bye" to exit chat */
		if(!(strncmp("bye", buff , 3)))
		{
			flag = 0;
			break;
		}
	}
	pthread_exit(NULL);
}

int create_socket()
{
	int sockfd = -1;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf("socket creation failed... exit code %d\n", sockfd);
		return sockfd;
	}
	else
	{
		printf("Socket successfully created.. FD: %d\n", sockfd);
		return sockfd;
	}
}

int setup_server(int server_fd,
				int *connfd,
				struct sockaddr_in *servaddr,
				struct sockaddr_in *cli,
				socklen_t *len)
{
	int ret = -1;
	int reuse1;

	/* to reuse socket with same address */
	reuse1 = 1;
	ret = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
			(const char *)&reuse1, sizeof(reuse1));
	if (ret < 0){
		perror("setsockopt(SO_REUSEADDR) failed");
		return ret;
	}

	/* clear the servaddr */
	memset(servaddr, '\0', sizeof(*servaddr));

	/* assign IP, PORT */
	servaddr->sin_family = AF_INET;
	/* long integer from host byte order to network byte order. */
	servaddr->sin_addr.s_addr = htonl(INADDR_ANY);
	/* short integer from host byte order to network byte order. */
	servaddr->sin_port = htons(PORT1);

	/* Binding newly created socket to given IP and verification */
	ret = bind(server_fd, (struct sockaddr*)servaddr, sizeof(*servaddr));
	if (ret != 0) {
		printf("socket bind failed... ret: %d\n", ret);
		return ret;
	}
	else
		printf("Socket successfully binded..\n");

	/* server listens */
	ret = listen(server_fd, 5);
	if (ret != 0) {
		printf("Listen failed...\n");
		return ret;
	}
	else
		printf("Server listening..\n");

	*len = sizeof(cli);

	/* Accept connection from client */
	*connfd = accept(server_fd, (struct sockaddr*)&cli, len);
	if (*connfd < 0) {
		printf("server accept failed...\n");
		return *connfd;
	}
	else{
		printf("server accepted the client...\n");
	}

	return 1;
}

int setup_client(int client_fd,
				struct sockaddr_in *cliaddr)
{
	int reuse = 1;
	int ret = -1;

	/* to reuse socket with same address */
	ret = setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));
	if (ret < 0){
		perror("setsockopt(SO_REUSEADDR) failed");
		return ret;
	}

	bzero(cliaddr, sizeof(*cliaddr));

	/* address structure */
	cliaddr->sin_family = AF_INET;
	cliaddr->sin_addr.s_addr = inet_addr("127.0.0.1");
	cliaddr->sin_port = htons(PORT2);

	/* connect to server */
	ret = connect(client_fd, (struct sockaddr *)cliaddr, sizeof(*cliaddr));
	if(ret < 0)
	{
		perror("connect");
		return ret;
	}
	else
		printf("connected to socket\n");

	return 0;
}

int multiThreadedChatFunction(SERVER_DATA *server_data,
								pthread_t *readThreadID,
								pthread_t *writeThreadID)
{
	int ret = -1;

	printf("\n================= Bidirectional Chat Started ================\n");
	printf("\nsend 'bye' to exit chat app\n");

	/* creating threads by default joinable */
	ret = pthread_create(readThreadID, NULL, reading, (void *)server_data);
	if(ret != 0)
	{
		perror("thread1");
		return -1;
	}
	ret = pthread_create(writeThreadID, NULL, writing, (void *)server_data);
	if(ret != 0)
	{
		perror("thread2");
		return -1;
	}

	/* Joining the threads */
	pthread_join(*readThreadID, NULL);
	pthread_join(*writeThreadID, NULL);

	printf("\n================= Bidirectional Chat ended ================\n");
	return 0;
}

int main()
{
	printf("Server App version: %s\n", SERVER_VERSION);

	int ret = -1;
	int serverFD = -1;
	int connectedClientFD = -1;
	int internalClientFD = -1;
	int running = 1;
	socklen_t len = -1;
	struct sockaddr_in servaddr, connectedClientAddr;
	struct sockaddr_in internalClientAddr;
	char choice_buff[MAX];
	SERVER_DATA *serverData;

	serverData = (SERVER_DATA *) malloc (sizeof(SERVER_DATA));
	if (serverData == NULL){
		printf("Cannot allocate memory for serverData\n");
		exit(1);
	}

	/* Threading implementation */
	pthread_t readingThread, writingThread;

	/* create server socket */
	serverFD = create_socket();
	if (serverFD < 0){
		printf("Failed creating server FD\n");
		exit(1);
	}

	/* create client socket */
	internalClientFD = create_socket();
	if (internalClientFD < 0){
		printf("Failed creating internal client FD\n");
		if (serverFD)
			close(serverFD);
		exit(1);
	}

	/* server setup */
	ret = setup_server(serverFD, &connectedClientFD, &servaddr, &connectedClientAddr, &len);
	if (ret < 0) {
		if (serverFD)
			close(serverFD);
		if (connectedClientFD)
			close(connectedClientFD);
		exit(1);
	}

	/* client setup */
	ret = setup_client(internalClientFD, &internalClientAddr);
	if (ret < 0) {
		if (serverFD)
			close(serverFD);
		if (connectedClientFD)
			close(connectedClientFD);
		exit(1);
	}

	serverData->mainServerFD = serverFD;
	serverData->mainClientFD = connectedClientFD;
	serverData->secondaryClientFD = internalClientFD;

	/* Infinite Loop */
	do
	{
		/* Read the choice from the client FD */
		bzero(choice_buff, MAX);

		/* Receive choice from client */
		int bytes_received = recv(connectedClientFD, choice_buff, MAX, 0);
		if (bytes_received <= 0){
			printf("Client disconnected\n");
		}
		//printf("choice_buff: %s", choice_buff);

		switch(atoi(choice_buff))
		{
			case CHAT:
				ret = multiThreadedChatFunction(serverData,
						&readingThread,
						&writingThread);
				if (ret < 0) {
					printf("Chat functionality exited with code %d\n", ret);
					running = 0;
					break;
				}
				break;

			case SEND_FILE:
                printf("\n\n======== Sending file to Server: ========\n\n");

                ret = sendFileToClient(internalClientFD);
                if (ret < 0) {
                    printf("Chat functionality exited with code %d\n", ret);
					running = 0;
					break;
				}
				break;

			case RECIEVE_FILE:
                printf("\n\n======== Recieving file from server: ========\n\n");

                ret = receiveFileFromClient(connectedClientFD);
                if (ret < 0) {
                    printf("Chat functionality exited with code %d\n", ret);
					running = 0;
					break;
				}
                break;

			case EXIT:
				printf("\nThank you for using this App.\n\n");
				running = 0;
				break;

			default:
				printf("\nInvalid choice received.\n");
		}
	}
	while(running);

	if (connectedClientFD)
		close(connectedClientFD);
	if (serverFD)
		close(serverFD);
	if (internalClientFD)
		close(internalClientFD);

	return 0;
}
