/*
 * seker_server.c
 *
 *  Created on: 22 Mar 2019
 *      Author: lital
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>


#include "Message.h"

#define CONNECTION_QUEUE_SIZE 100
#define MAX_RATING_LEN 1024
#define MAX_RATING_VAL_LEN 3
#define MAX_COURSE_NAME_LEN 100
#define MAX_COURSE_NUMBER_LEN 4
#define MAX_USER_LEN 15
#define MAX_PASS_LEN 15
#define MAX_USERS 25
#define MAX_PATH 1000

#define WELCOME_MSG "Welcome! Please log in.\n"
#define FILE_COURSES "courses.txt"

int listenfd; //listening socket file descriptor
char path[MAX_PATH];

typedef struct User{
	char userName[MAX_USER_LEN+1];
	char password[MAX_PASS_LEN+1];
} User;

typedef struct Client{
	bool isValid;

	int sd;

	bool isLoggedIn;
	User details;

} Client;

int getUsersList(char* users_file, User* users, int* len);
int isCourseExist(char* num, bool* res);
int isCourseExist(char* num, bool* res);
int addCourse(char* num, char* name, bool* added);
int rateCourse(char* user_name, char* course_num, char* rating_val, char* rating_txt);
bool isLoginCorrect (User* users, int len, User u);
int getFileSize (FILE* fp);
int sendToClient(int s, uint8_t is_last, MESSAGE_TYPE msg_type, char* data);
int sendFileToClient (int s, MESSAGE_TYPE msg_type, char* file_name);
int handleListOfCourses(int s);
int handleAddCourse(int s, Message msg, Client* clients);
int handleGetRate(int s, Message msg);
int handleRateCourse(int s, Message msg, char* user_name);
int handleRequest(int s, Message msg, char* user_name, Client* clients);
int getUserDetails(int s, User* u);
int registerHandler(int signal, struct sigaction *action, void (*sa_action) (int, siginfo_t *, void *));
void sigpipeHandler();
void initFDs (int listenfd, Client* clients, fd_set* read_fds, fd_set* write_fds, int* fdmax);
Client newClient(int sd);
int handleNewConnection(int listenfd, Client* clients);
int handleLogin (int sd, User* details, bool* isLoggedIn, User* users, int numUsers);

/**Registers the handler function to the specified signal
 *
 * @param signal - the signal to be handled
 * @param action - structure to pass to the registration syscall
 * @param handlerFunc - the handler function to the specified signal
 *
 * @return
 * -1 on error
 * 0 on success*/
int registerHandler(int signal, struct sigaction *action, void (*sa_action) (int, siginfo_t *, void *)){
	action->sa_sigaction = sa_action;
	action->sa_flags = SA_SIGINFO;
	sigfillset(&action->sa_mask);
	// Register the handler to the signal
	if( 0 != sigaction(signal, action, NULL) ){
		return -1;
	}
	return 0;
}

/**Handles a SIGPIPE signal*/
void sigpipeHandler(){
}

/**Stores the content of the users_file in the users array
 *
 * @param users_file - name of the file conatains the user names and passwords
 * @param users - will get the content
 * @param len - will get the length of the users array
 *
 * @return
 * -1 - on error
 * 0 -on success */
int getUsersList(char* users_file, User* users, int* len){
	FILE* fp = fopen(users_file, "r");
	if (fp == NULL){
		perror("Failed opening file");
		return -1;
	}

	int i = 0;
	while (fscanf(fp, "%s", users[i].userName) > 0) {
		fscanf(fp, "%s", users[i].password);
		i++;
	}

	*len = i;
	fclose(fp);
	return 0;
}

/**Checks if the course exists in the database
 *
 * @param num - the course number
 * @param res - will be set to true if the course exists and false otherwise.
 *
 * @return
 * -1 - on error
 * 0 - on success*/
int isCourseExist(char* num, bool* res){
	char buf[MAX_COURSE_NUMBER_LEN+MAX_COURSE_NAME_LEN+5];
	char* curr;
	char path_to_courses[MAX_PATH+50];
	sprintf(path_to_courses,"%s/%s", path, FILE_COURSES);

	FILE* fp = fopen(path_to_courses, "r");
	if (fp == NULL){
		return -1;
	}
	*res = false;
	while (fscanf(fp,"%s",buf) != EOF) {
		curr = strtok(buf, ":");
		if(strcmp(num, curr) == 0){
			*res = true;
		}
	}
	fclose(fp);
	return 0;
}

/**If the course number doesn't exist,
 * adds the course specified to the database ,
 * also, creates a new file for store its rates.
 *
 * @param num - the course number to be added
 * @param name - the course's name
 * @param added - will be set to true iff the course was actually added
 *
 * @return
 * -1 - on error
 * 0 - on success
 * */
int addCourse(char* num, char* name, bool* added){
	bool isExist;
	if (isCourseExist(num, &isExist) == -1){
		return -1;
	}
	if(!isExist){
		char path_to_courses[MAX_PATH+50];
		sprintf(path_to_courses,"%s/%s", path, FILE_COURSES);
		char path_to_new_course[MAX_PATH+50];
		sprintf(path_to_new_course,"%s/%s", path, num);

		FILE* fp = fopen(path_to_courses, "a");
		if (fp == NULL){
			return -1;
		}

		fprintf(fp, "%s:\t\"%s\"\n", num, name);
		fclose(fp);
		//create new file for the course(will be used for for storing rates)
		fp = fopen(path_to_new_course, "ab+");
		fclose(fp);

		*added = true;
	}
	else {
		*added = false;
	}


	return 0;
}

/**Adds the rate specified to the file contains this course rates.
 *
 * @param user_name - the name of the user rating the course
 * @param course_num - the course's number
 * @param rating_val - the rating value
 * @param rating_txt - the rating text
 *
 * @return
 * -1 - on error
 * 0 - on success
 * */
int rateCourse(char* user_name, char* course_num, char* rating_val, char* rating_txt){
	char new_path[MAX_PATH+MAX_COURSE_NUMBER_LEN+1];

	//open the file conatains the rates for the course
	sprintf(new_path,"%s/%s", path, course_num);
	FILE* fp = fopen(new_path, "a");
	if (fp == NULL){
		return -1;
	}
	//write the rate to the file
	fprintf(fp, "%s:\t%s\t\"%s\"\n", user_name, rating_val, rating_txt);

	fclose(fp);

	return 0;
}

/**Checks if the user name and password are correct
 *
 * @param users - array contains the users and passwords of the clients in the system
 * @param len - the lenfth of the users array
 * @param u - the user details
 *
 * @return
 * true - if the details of the user are correct
 * false - otherwise*/
bool isLoginCorrect (User* users, int len,  User u){
	for (int i=0; i<len; i++){
		if(strcmp(users[i].userName, u.userName)==0 && strcmp(users[i].password, u.password)==0){
			return true;
		}
	}
	return false;
}

/**Returns size of the file
 *
 * @param fp -pointer to the file
 *
 * @return
 * The size (in bytes) of the file*/
int getFileSize (FILE* fp){
	fseek(fp, 0L, SEEK_END);
	int len = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	return len;
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
int sendToClient(int s, uint8_t is_last, MESSAGE_TYPE msg_type, char* data){
	Message msg;
	memset(&msg, 0 , sizeof(msg));
	uint32_t len = data==NULL? 0 : strlen(data);

	msg.header.type = msg_type;
	msg.header.is_last = is_last;
	msg.header.data_len = len;
	strncpy(msg.data, data, len);

	//send message data
	if (sendMessage(s, &msg) < 0){//error
		return -1;
	}
	return 0;
}


/**Sends the file content over the socket
 *
 * @param s - socket identifier
 * @param msg_type - type of the message
 * @param file_name - the name of the file tp be sent
 *
 * @return -1 on error
 * 			0 on success*/
int sendFileToClient (int s, MESSAGE_TYPE msg_type, char* file_name){
	FILE* fp;
	int len, bytesRead;
	uint8_t is_last = 0;
	char buffer[MAX_DATA_LEN+1];

	fp = fopen (file_name, "r");
	if (fp == NULL){//error
		return -1;
	}

	//get file size
	len = getFileSize(fp);
	if (len == -1){//error
		fclose(fp);
		return -1;
	}
	if (len == 0){
		if (sendToClient(s, 1, msg_type, "") < 0){
			fclose(fp);
			return -1;
		}
		return 0;
	}


	//send file content to client
	while ((bytesRead = fread(buffer, sizeof(char), MAX_DATA_LEN, fp)) > 0){
		len -= bytesRead;
		if (len == 0) {
			is_last = 1;
		}
		//send the buffer to client
		buffer[bytesRead] = '\0';
		if (sendToClient(s, is_last, msg_type, buffer) < 0){
			fclose(fp);
			return -1;
		}
	}

	fclose(fp);

	if (len != 0) {//error
		return -1;
	}

	return 0;
}

/**Send the list of courses stored in the server over the socket specified
 *
 * @param s - socket identifier
 * @return
 * 0 - on success
 * -1 - on error*/
int handleListOfCourses(int s){
	char path_to_courses[MAX_PATH+50];
	sprintf(path_to_courses,"%s/%s", path, FILE_COURSES);
	//send the list of courses to client
	int status = sendFileToClient(s, LIST_OF_COURSES, path_to_courses);
	if (status == -1){
		return -1;
	}
	return 0;
}

/**Parses the message arguments,
 * adds the course specified to the server's database if it doesn't exist,
 * and sends an appropiate response over the socket.
 *
 * @param s - socket identifier
 * @param msg - the message contains the course details
 *
 * @return
 * 0 - on success
 * -1 - on error*/
int handleAddCourse(int s, Message msg, Client* clients){
	char course_number[MAX_COURSE_NUMBER_LEN+1];
	char course_name[MAX_COURSE_NAME_LEN+1];
	char response[MAX_DATA_LEN+100];
	int status;
	bool added;

	//parse arguments
	char delimiter[] = "\n";
	strcpy(course_number,strtok(msg.data, delimiter)); //get first token (course_number)
	strcpy(course_name, strtok(NULL, delimiter)); //get next token (course_name)


	//add the course to the courses file
	status = addCourse(course_number, course_name, &added);
	if (status == -1){
		return -1;
	}

	if (added){ //the course was succcessfully added
		sprintf(response, "%s added successfully.\n", course_number);
		//send response to client
		status = sendToClient(s, 1, ADD_COURSE, response);
		//send to all the other loggedin users that a new course was added
		for (int i = 0; i < MAX_USERS; i++) {
			if (clients[i].isValid && clients[i].sd!=s && clients[i].isLoggedIn){
				status = sendToClient(clients[i].sd, 1, ADD_COURSE, "A new course was just added!\n");
				if (status == -1){
					return -1;
				}
			}
		}
	}
	else { //the course is already exists
		sprintf(response, "%s exists in the database!\n", course_number);
		//send response to client
		status = sendToClient(s, 1, ADD_COURSE, response);
	}

	//send response to client
	//status = sendToClient(s, 1, ADD_COURSE, response);
	if (status == -1){
		return -1;
	}

	return 0;
}

/**Parses the message arguments,
 * adds the course specified to the server's database if it doesn't exist,
 * and sends an appropiate response over the socket.
 *
 * @param s - socket identifier
 * @param msg - the message contains the course details
 *
 * @return
 * 0 - on success
 * -1 - on error*/
int handleBroadcast(int s, Message msg, char* user_name, Client* clients){
	char response[MAX_DATA_LEN+100];
	int status;

	sprintf(response, "%s sent a new message: %s\n", user_name, msg.data);

	for (int i = 0; i < MAX_USERS; i++) {
		//send to all the other loggedin users the message
		if (clients[i].isValid && clients[i].sd!=s && clients[i].isLoggedIn){
			status = sendToClient(clients[i].sd, 1, ADD_COURSE, response);
			if (status == -1){
				return -1;
			}
		}
	}

	return 0;

}

/**Parses the message arguments,
 * If the course exists, sends its rates over the socket,
 * Otherwise, sends "The course doesn't exist in the database!\n"
 *
 * @param s - socket identifier
 * @param msg - the message contains the course details
 *
 * @return
 * 0 - on success
 * -1 - on error*/
int handleGetRate(int s, Message msg){
	char new_path[MAX_PATH+MAX_DATA_LEN+1];
	char course_number[MAX_COURSE_NUMBER_LEN+1];
	bool is_course_exist = false;
	int status = 0;

	strcpy(course_number ,msg.data);

	//check if the course exists
	status = isCourseExist(course_number, &is_course_exist);
	if (status == -1){
		return -1;
	}

	if (!is_course_exist){
		status = sendToClient(s, 1, RATE_COURSE, "The course doesn't exist in the database!\n");
	}
	else {
		//the path for the file conataing the rates for this course
		sprintf(new_path,"%s/%s", path, course_number);
		status = sendFileToClient(s, GET_RATE, new_path);
	}

	return status;

}

/**Parses the message arguments,
 * If the course exists, adds the rating specified to the course rates,
 * Otherwise, sends "The course doesn't exist in the database!\n"
 *
 * @param s - socket identifier
 * @param msg - the message contains the rating details
 * @param user_name - the nme of the user rating the course
 *
 * @return
 * 0 - on success
 * -1 - on error*/
int handleRateCourse(int s, Message msg, char* user_name){
	char course_number[MAX_COURSE_NUMBER_LEN+1];
	char rating_val[MAX_RATING_VAL_LEN+1];
	char rating_txt[MAX_RATING_LEN+1];
	char* response = NULL;
	bool is_course_exist;
	int status;

	//parse arguments
	char delimiter[] = "\n";
	strcpy(course_number,strtok(msg.data, delimiter)); //get first token (course_number)
	strcpy(rating_val, strtok(NULL, delimiter)); //get next token (rating_val)
	strcpy(rating_txt, strtok(NULL, "")); //get the last token (rating_txt)

	//check if the course exists in the database
	status = isCourseExist(course_number, &is_course_exist);
	if (status == -1){
		return -1;
	}

	if (is_course_exist){
		status = rateCourse(user_name, course_number, rating_val, rating_txt);
		if (status == -1){
			return -1;
		}
	}
	else {
		response = "The course doesn't exist in the database!\n";
	}

	//send response to client
	status = sendToClient(s, 1, RATE_COURSE, response);
	if (status == -1){
		return -1;
	}

	return 0;
}

/**Handles the request specified
 *
 * @param s - socket identifier
 * @param msg - the request
 * @param user_name - the name of the user sent the request
 *
 * @return
 * 0 - on success
 * -1 - on error*/
int handleRequest(int s, Message msg, char* user_name, Client* clients){
	int status = 0;
	if (msg.header.type == LIST_OF_COURSES){
		status = handleListOfCourses(s);
	}
	else if (msg.header.type == ADD_COURSE){
		status = handleAddCourse(s, msg, clients);
	}
	else if (msg.header.type == GET_RATE){
		status = handleGetRate(s, msg);
	}
	else if (msg.header.type == RATE_COURSE){
		status = handleRateCourse(s, msg, user_name);
	}
	else if (msg.header.type == BROADCAST){
		status = handleBroadcast(s, msg, user_name, clients);
	}
	return status;
}

/**Receives from the socket a login message,
 * parses it and store the results in the pointer passed.
 *
 * @param s - socket identifier
 * @param u - will get the results
 *
 * @return
 * -1 - on error
 * 0 - on success
 * */
int getUserDetails(int s, User* u){
	Message req;
	char delimiter[] = "\n";

	int status = recvMessage(s, &req);
	if (status == -1){
		return -1;
	}


	strcpy(u->userName,strtok(req.data, delimiter)); //get first token (userName)
	strcpy(u->password, strtok(NULL, delimiter)); //get next token (password)

	return 0;
}

void initFDs (int listenfd, Client* clients, fd_set* read_fds, fd_set* write_fds, int* fdmax){
	//reset fd sets
	FD_ZERO(read_fds);
	FD_ZERO(write_fds);

	FD_SET(listenfd, read_fds); // add listening socket
	*fdmax = listenfd;
	int sd;
	for (int i=0; i<MAX_USERS; i++) {
		//socket descriptor
		if (clients[i].isValid){
			sd = clients[i].sd;
			FD_SET(sd , read_fds);
			FD_SET(sd , write_fds);
			//highest file descriptor number
			if(sd > *fdmax) {
				*fdmax = sd;
			}

		}
	}
}

Client newClient(int sd){
	Client c;
	c.sd = sd;
	c.isValid = true;
	c.isLoggedIn = false;

	return c;
}

int handleNewConnection(int listenfd, Client* clients){
	//accept new connection
	struct sockaddr_in client_addr;
	socklen_t client_addr_size = sizeof(struct sockaddr_in);
	int new_socket = accept(listenfd, (struct sockaddr*)&client_addr, &client_addr_size);
	if (new_socket < 0)
	{
		return -1;
	}

	//add new client
	for (int i = 0; i < MAX_USERS; i++) {
		//if position is empty
		if(!clients[i].isValid)
		{
			clients[i] = newClient(new_socket);
			//send welcome message
			if (sendToClient(new_socket, 1, WELCOME, WELCOME_MSG) < 0){
				return -1;
			}
			break;
		}
	}

	return 0;

}

int handleLogin (int sd, User* details, bool* isLoggedIn, User* users, int numUsers){
	//get login details
	if (getUserDetails(sd, details) == -1){
		return -1;
	}
	//check if login details are correct
	*isLoggedIn = isLoginCorrect(users, numUsers, *details);
	if (!*isLoggedIn){
		//Login failed
		if (sendToClient(sd, 1, LOGIN_FAIL, "Failed to login.\n") < 0){
			return -1;
		}
	}
	else{
		//login succeeded
		char suceessLoginMsg[MAX_USER_LEN+50];
		sprintf(suceessLoginMsg, "Hi %s, good to see you.\n", details->userName);
		//Login failed
		if (sendToClient(sd, 1, LOGIN_SUCCESS, suceessLoginMsg) < 0){
			return -1;
		}
	}
	return 0;
}

int main(int argc, char *argv[]) {
	setvbuf(stdout, NULL, _IONBF, 0);
	struct sigaction sigpipe_action;
	memset(&sigpipe_action, 0, sizeof(sigpipe_action));
	//register the handler to SIGPIPE
	if (registerHandler(SIGPIPE, &sigpipe_action, sigpipeHandler) < 0){
		exit(EXIT_FAILURE);
	}

	//command line arguments
	if (argc < 3){
		printf("Usage: ./seker_server users_file dir_path [port]\n");
		exit(EXIT_FAILURE);
	}
	char *users_file = argv[1];
	char *dir_path = argv[2];
	unsigned int port = 1337;
	if (argc >= 4){
		port = strtoul(argv[3], NULL, 10);
		if (port == 0 && strcmp(argv[3], "0") != 0){
			printf("The port specified %s is not a number\n",argv[3]);
			exit(EXIT_FAILURE);
		}
	}
	strcpy(path, dir_path);

	//create new file for storing the available courses
	char path_to_courses[MAX_PATH+50];
	sprintf(path_to_courses,"%s/%s", path, FILE_COURSES);
	FILE *fp = fopen(path_to_courses, "ab+");
	fclose(fp);

	//get all users details
	User users[MAX_USERS];
	int numUsers;
	if (getUsersList(users_file, users, &numUsers) == -1){
		perror("Failed getting users details");
		exit(EXIT_FAILURE);
	}

	//create a listening socket
	listenfd = socket(PF_INET, SOCK_STREAM, 0);
	if (listenfd == -1) {
		perror("Failed creating listening socket");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);

	//bind socket to specified port
	if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in)) == -1) {
		perror("Failed binding socket");
		close(listenfd);
		exit(EXIT_FAILURE);
	}

	if (listen(listenfd, CONNECTION_QUEUE_SIZE) == -1) {
		perror("Failed start listening to incoming connections");
		close(listenfd);
		exit(EXIT_FAILURE);
	}


	fd_set read_fds, write_fds;
	Client clients[MAX_USERS];
	memset(&clients, 0 ,sizeof(clients));


	int sd, fdmax;

	//main loop
	while (1) {
		//INIT Step
		initFDs(listenfd, clients, &read_fds, &write_fds, &fdmax);

		int activity = select(fdmax + 1, &read_fds, &write_fds, NULL, NULL);
		if (activity < 0) {
			perror("select");
			exit(EXIT_FAILURE);
		}

		if (FD_ISSET(listenfd, &read_fds)) {
			//listening socket is read-ready- can accept
			if (handleNewConnection(listenfd, clients) < 0){
				perror("new connection");
				exit(EXIT_FAILURE);
			}
		}

		//some IO operation on some other socket
		for (int i = 0; i < MAX_USERS; i++) {
			if (!clients[i].isValid){
				continue;
			}

			sd = clients[i].sd;

			//read
			if (FD_ISSET(sd , &read_fds)) {
				//recv
				if (!clients[i].isLoggedIn) {
					if (handleLogin(sd, &clients[i].details, &clients[i].isLoggedIn, users, numUsers) < 0){
						perror("Failed handling login");
						exit(EXIT_FAILURE);
					}
				}
				else {
					//get client requests and respond accordingly
					Message req;
					int status = recvMessage(sd, &req);
					if (status == -1){
						perror("Failed receiving data from client");
						exit(EXIT_FAILURE);
					}
					if (status == -2){ //client closed connection
						//Close the socket and mark as invalid for reuse
						close(sd);
						clients[i].isValid = false;
					}
					if (req.header.type == QUIT){
						//Close the socket and mark as invalid for reuse
						close(sd);
						clients[i].isValid = false;
					}
					else {
						if (handleRequest(sd, req, clients[i].details.userName, clients) < 0){
							perror("Failed handling request");
							exit(EXIT_FAILURE);
						}
					}
				}
			}
		}
	}

	close(listenfd);
	exit(EXIT_SUCCESS);
}

