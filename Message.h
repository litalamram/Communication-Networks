/*
 * Message.h
 *
 *  Created on: 26 Mar 2019
 *      Author: lital
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <stdint.h>


#define MAX_DATA_LEN 2048

//Used to reperesent a message type
typedef enum MESSAGE_TYPE {
	WELCOME,
	LOGIN,
	LOGIN_SUCCESS,
	LOGIN_FAIL,
	LIST_OF_COURSES,
	ADD_COURSE,
	RATE_COURSE,
	GET_RATE,
	BROADCAST,
	QUIT,
} MESSAGE_TYPE;

//A new type that is used to represent the header of a message
typedef struct Header {
	uint8_t type;
	uint8_t is_last;
	uint32_t data_len;
} Header;

//A new type that is used to represent a message corresponding to the server-client protocol
typedef struct Message {
	Header header;
	char data[MAX_DATA_LEN];
} Message;

/**Sends the whole message over the socket
 *
 * @param s - the socket identifier
 * @param msg - the message to be sent
 *
 * @return
 * -1 - on failure
 * 0 - on success*/
int sendMessage (int s, Message* m);

/**Receives the whole message from the socket
 *
 * @param s - the socket identifier
 * @param msg - stores the results
 *
 * @return
 * -1 - on failure
 * 0 - on success*/
int recvMessage (int s, Message* m);

/**Prints the data of the message*/
void printMessage (Message m);

#endif /* MESSAGE_H_ */
