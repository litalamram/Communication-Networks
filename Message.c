/*
 * Message.c
 *
 *  Created on: 26 Mar 2019
 *      Author: lital
 */


#include "Message.h"

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>



/**Sends len bytes from the buffer over the socket specified
 *
 * @param s - socket identifier
 * @param buf - buffer to be sent
 * @param len - number of bytes to send from the buffer
 *
 * @return
 * -1 - on failure
 * 0 - on success*/
int sendall(int s, char* buf, uint32_t len) {
	uint32_t total = 0; // how many bytes we've sent
	int bytesleft = len; // how many we have left to send
	int n = 0;
	while(total < len) {
		n = send(s, buf+total, bytesleft, 0);
		if (n == -1) { break; }
		total += n;
		bytesleft -= n;
	}
	if (n==-1 && errno == EPIPE){
		return 0;
	}
	return n == -1 ? -1:0; //-1 on failure, 0 on success
}

/**Receives len bytes from the socket, storing the results in the buffer.
 *
 * @param s - socket identifier
 * @param buf - buffer to store the results
 * @param len - number of bytes to read from the socket
 *
 * @return
 * -1 - on failure
 * 0 - on success*/
int recvall(int s, char *buf, uint32_t len) {
	uint32_t total = 0; // how many bytes we've read
	int bytesleft = len; // how many we have left to read
	int n = 0;
	while(total < len) {
		n = recv(s, buf+total, bytesleft, 0);
		if (n == -1) { break; }
		if (n == 0) {return -2;}//n = -1; break;} // the other side closed the connection
		total += n;
		bytesleft -= n;
	}
	return (n == -1) ? -1:0; //-1 on failure, 0 on success
}

/**Receives the header of the message  from the socket
 *
 * @param s - the socket identifier
 * @param header - to store the results
 *
 * @return
 * -1 - on failure
 * 0 - on success*/
int recvHeader(int s, Header* header){
	int status;
	status = recvall(s, (char*)header, sizeof(Header));
	//convert to host order
	header->data_len = ntohl(header->data_len);
	return status;
}

/**Sends the header of the message over the socket
 *
 * @param s - the socket identifier
 * @param header - the header to be sent
 *
 * @return
 * -1 - on failure
 * 0 - on success*/
int sendHeader(int s, Header header){
	int status;
	//convert to network order
	header.data_len = htonl(header.data_len);
	status = sendall(s, (char*)&header, sizeof(Header));
	return status;
}

/**Sends the whole message over the socket
 *
 * @param s - the socket identifier
 * @param msg - the message to be sent
 *
 * @return
 * -1 - on failure
 * 0 - on success*/
int sendMessage(int s, Message* msg){
	int status;
	//send header
	status = sendHeader(s, msg->header);
	if (status == -1){
		return -1;
	}
	//send msg
	status = sendall(s, msg->data, msg->header.data_len);

	return status;
}

/**Receives the whole message from the socket
 *
 * @param s - the socket identifier
 * @param msg - stores the results
 *
 * @return
 * -1 - on failure
 * 0 - on success*/
int recvMessage(int s, Message* msg){
	int status;
	//receive header
	status = recvHeader(s, &msg->header);
	if (status == -1){
		return -1;
	}
	//receive msg data
	status = recvall(s, msg->data, msg->header.data_len);
	msg->data[msg->header.data_len] = '\0';

	return status;
}

/**Prints the data of the message*/
void printMessage (Message m){
	printf("%s", m.data);
}
