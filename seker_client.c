/*
 * seker_client.c
 *
 *  Created on: 22 Mar 2019
 *      Author: lital
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>


#include "Command.h"
#include "Message.h"

void freeResources();
void handleError(const char *msg);
int sendToServer(int s, MESSAGE_TYPE msg_type, char* data);
int printServerResponse(int sockfd);
Command getCommandFromUser();
int handleLogin(int s, Command cmd, bool* success);
int handleCommand(int s, Command cmd);


int sockfd = -1;
#define STDIN 0
/** Frees all the memory associated with the program */
void freeResources(){
	if (sockfd != -1){
		close(sockfd);
	}
}

/**Prints the error massage specified,
 * frees the memory ,
 * and exits with EXIT_FAILURE
 *
 * @param msg - the error massage to print
 * */
void handleError(const char *msg) {
	fprintf(stderr,"Error: %s: %s\n", msg, strerror(errno));
	freeResources();
	exit(EXIT_FAILURE);
}

/**Sends message over the socket
 *
 * @param s - socket identifier
 * @param is_last - is it the last message of the stream
 * @param msg_type - the type of the message
 * @param data - the message's data, should be ended with '\0'
 *
 * @return
 * -1 - on error
 * 0 - on success
 * */
int sendToServer(int s, MESSAGE_TYPE msg_type, char* data){
	Message msg;
	memset(&msg, 0 , sizeof(msg));
	uint32_t len = data==NULL? 0 : strlen(data);

	msg.header.type = msg_type;
	msg.header.is_last = 1;
	msg.header.data_len = len;
	strncpy(msg.data, data, len);

	//send message data
	if (sendMessage(s, &msg) < 0){//error
		return -1;
	}
	return 0;
}

/**Receives stream of messages from the socket (untill the field is_last of a message is 1),
 * and prints their content to stdout
 *
 * @param sockfd - socket identifier
 *
 * @return
 * -1 - on error
 * 0 - on success*/
int printServerResponse(int sockfd){
	int status;
	Message m;
	memset(&m, 0, sizeof(m));

	do {
		status = recvMessage(sockfd, &m);
		if (status == -1){
			return -1;
		}
		printMessage(m);
	} while (!m.header.is_last);

	return 0;
}

/**Gets input from the user and parses it into an appropriate Command
 *
 * @return the parsed command*/
Command getCommandFromUser(){
	char command[MAX_LINE_LEN+1];
	if (fgets(command, MAX_LINE_LEN, stdin) == NULL){
		handleError("Error getting user's input");
	}
	return parseLine(command);
}

/**Sends a LOGIN message over the given socket,
 * gets and prints the response for it,
 * and sets the success argument to true iff the login succeeded
 *
 * @param s - the socket identifier.
 * @param cmd - the Command of type CMD_LOGIN
 * @param success - sets to true iff the login operation succeeded
 *
 * @return
 * -1 - on error
 * 0 - on success*/
int handleLogin(int s, Command cmd, bool* success){
	int status;

	//send user details to server
	status = sendToServer(s, LOGIN, cmd.args);
	if (status == -1){
		return -1;
	}

	//get server's response
	Message response;
	status = recvMessage(s, &response);
	if (status == -1){
		return -1;
	}

	//print server's response
	printMessage(response);

	//are login details correct
	*success = response.header.type == LOGIN_SUCCESS;

	return 0;
}

/**If the given command is valid, sends it over the socket.
 * Otherwise, prints "Illegal command.\n".
 *
 * @param - socket identifier
 * @param cmd - the command to be handled
 *
 * @return
 * -1 - on error
 * 0 - on success*/
int handleCommand(int s, Command cmd){
	int status;
	MESSAGE_TYPE mtype;
	char* data = NULL;
	if (cmd.cmd == CMD_INVALID || !cmd.validArgs){
		printf("Illegal command.\n");
		return 0;
	}
	else if (cmd.cmd == CMD_LIST_OF_COURSES){
		status = sendToServer(s, LIST_OF_COURSES, NULL);
		if (status == -1){
			return -1;
		}
		//get server's response
		status = printServerResponse(s);
		if (status == -1){
			return -1;
		}
	}
	else if (cmd.cmd == CMD_ADD_COURSE){
		status = sendToServer(s, ADD_COURSE, cmd.args);
		if (status == -1){
			return -1;
		}
		//print server's response
		status = printServerResponse(s);
		if (status == -1){
			return -1;
		}
	}
	else if (cmd.cmd == CMD_RATE_COURSE){
		status = sendToServer(s, RATE_COURSE, cmd.args);
		if (status == -1){
			return -1;
		}

		//print server's response
		status = printServerResponse(s);
		if (status == -1){
			return -1;
		}
	}
	else if (cmd.cmd == CMD_GET_RATE){
		status = sendToServer(s, GET_RATE, cmd.args);
		if (status == -1){
			return -1;
		}
		//get server's response
		status = printServerResponse(s);
		if (status == -1){
			return -1;
		}
	}

	else if (cmd.cmd == CMD_BROADCAST){
		mtype = BROADCAST;
		data = cmd.args;
		status = sendToServer(s, mtype, data);
		if (status == -1){
			return -1;
		}
	}
	else if (cmd.cmd == CMD_QUIT){
		mtype = QUIT;
		status = sendToServer(s, mtype, data);
		if (status == -1){
			return -1;
		}
	}


	return 0;
}

int main(int argc, char *argv[]) {
	setvbuf(stdout, NULL, _IONBF, 0);

	//command line arguments
	char *host = "localhost";
	char *port = "1337";

	if (argc > 1){
		host = argv[1];
		if (argc > 2){
			port = argv[2];
		}
	}
	//create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		handleError("Failed creating socket");
	}

	//parse host to ip and connect to host
	struct addrinfo *result;
	int s = getaddrinfo(host, port, NULL, &result);
	if (s != 0) {
		handleError("Erron in getaddrinfo()");
	}
	//try each address getaddrinfo() returned until we successfully connect.
	struct addrinfo *rp;
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) {//success
			break;
		}

	}
	freeaddrinfo(result);

	//no address succeeded
	if (rp == NULL) {
		handleError("Could not connect");
	}

	//Welcome msg
	int status = printServerResponse(sockfd);
	if (status == -1){
		handleError("Failed receving data from server");
	}

	Command cmd;

	//login
	bool login_success = false;
	char user_line[MAX_LINE_LEN+1];
	char pass_line[MAX_LINE_LEN+1];
	do {

		if (fgets(user_line, MAX_LINE_LEN, stdin) == NULL){
			handleError("Failed getting user's input");
		}
		if (fgets(pass_line, MAX_LINE_LEN, stdin) == NULL){
			handleError("Failed getting user's input");
		}

		cmd = parseLogin(user_line, pass_line);

		if(cmd.cmd == CMD_INVALID){
			printf("Failed to login.\n");
		}
		else{
			status = handleLogin(sockfd, cmd, &login_success);
			if (status == -1){
				handleError("Error handling login");
			}
		}
	} while (!login_success);

	//get commands from client untill he asks to quit
	fd_set read_fds, write_fds;
	int fdmax = sockfd > STDIN? sockfd: STDIN;
	bool to_send = false;
	while (true){
		//reset fd set
		FD_ZERO(&read_fds);
		FD_ZERO(&write_fds);
		//add fds
		FD_SET(STDIN, &read_fds);
		FD_SET(sockfd, &read_fds);
		FD_SET(sockfd, &write_fds);

		int activity = select(fdmax + 1, &read_fds, &write_fds, NULL, NULL);
		if (activity < 0 && errno!=EINTR) {
			handleError("select error");
		}
		//user input
		if (!to_send && FD_ISSET(STDIN, &read_fds)) {
			cmd = getCommandFromUser();
			to_send = true;
		}
		//send request to server
		if (to_send && FD_ISSET(sockfd, &write_fds)) {
			//print server's response
			status = handleCommand(sockfd, cmd);
			if (status == -1) {
				handleError("Error sending to the server");
			}
			to_send = false;
			if (cmd.cmd == CMD_QUIT){
				break;
			}
		}
		//server sent message
		if (FD_ISSET(sockfd, &read_fds)) {
			//print server's response
			status = printServerResponse(sockfd);
			if (status == -1){
				handleError("Error receiving from server");
			}
		}
	}


	freeResources();

	return EXIT_SUCCESS;
}
