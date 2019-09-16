/*
 * Command.c
 *
 *  Created on: 24 Mar 2019
 *      Author: lital
 */
#include "Command.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define USER_PREFIX_TXT "User:"
#define PASSWORD_PREFIX_TXT "Password:"
#define LIST_OF_COURSES_TXT "list_of_courses"
#define ADD_COURSE_TXT "add_course"
#define RATE_COURSE_TXT "rate_course"
#define GET_RATE_TXT "get_rate"
#define BROADCAST_TXT "broadcast"
#define QUIT_TXT "quit"


Command_Type parseCommand(const char* str);
bool parseInt(const char* str, int* n);
bool isValidCourseNum (int n);
bool isValidRatingVal (int n);
void parseAddCourse(Command* cmd, char* course_num, char* course_name);
void parseRateCourse(Command* cmd, char* course_num, char* rating_val, char* rating_txt);
void parseGetRate(Command* cmd, char* course_num);

/**Parses a specified string to a command type.
 *
 * @return
 * the command type if the string is valid , and CMD_INVALID if it's invalid
 */
Command_Type parseCommand(const char* str){
	if (strcmp(str, LIST_OF_COURSES_TXT)==0){
		return CMD_LIST_OF_COURSES;
	}
	else if (strcmp(str, ADD_COURSE_TXT)==0){
		return CMD_ADD_COURSE;
	}
	else if (strcmp(str, RATE_COURSE_TXT)==0){
		return CMD_RATE_COURSE;
	}
	else if (strcmp(str, GET_RATE_TXT)==0){
		return CMD_GET_RATE;
	}
	else if (strcmp(str, BROADCAST_TXT)==0){
		return CMD_BROADCAST;
	}
	else if (strcmp(str, QUIT_TXT)==0){
		return CMD_QUIT;
	}
	return CMD_INVALID;
}

/**Parses the given str to int
 * @return true - if the conversion succeeded
 *         false - otherwise*/
bool parseInt(const char* str, int* n){
	*n = atoi(str);
	if (*n == 0 && strcmp(str, "0") != 0){
		return false;
	}
	return true;
}

/**Returnes true iff the given int is between 0 to 9999*/
bool isValidCourseNum (int n){
	return n>=0 && n<=9999;
}

/**Returns true iff the given integer is between 0 to 100*/
bool isValidRatingVal (int n){
	return n>=0 && n<=100;
}

/**Parses the arguments for CMD_LOGIN.
 * If the arguments are valid user name and password,
 * the field validArgs is set to true and the arguments are saved in the args field.
 * Otherwise, the field validArgs is set to false.
 *
 * @param cmd - the command
 * @param user_line, pass_line - the command arguments
 * */
Command parseLogin(const char* user_line, char* pass_line){
	Command command;
	command.cmd = CMD_INVALID;
	command.validArgs = false;
	if(user_line == NULL || pass_line == NULL){
		command.cmd = CMD_INVALID;
		return command;
	}
	char delimiter[] = " \t\r\n";
	char copy_user[MAX_LINE_LEN+1];
	char copy_pass[MAX_LINE_LEN+1];

	strcpy(copy_user, user_line);
	strcpy(copy_pass, pass_line);

	char *token = strtok(copy_user, delimiter);	//get first token (USER_PREFIX)
	if (token == NULL || strcmp(token, USER_PREFIX_TXT) != 0){
		return command;
	}
	char* userName = strtok(NULL, delimiter);  //get next token (user name)

	token = strtok(copy_pass, delimiter);  //get next token (PASSWORD_PREFIX)
	if (token == NULL || strcmp(token, PASSWORD_PREFIX_TXT) != 0){
		return command;
	}

	char* password = strtok(NULL, delimiter);  //get next token (password)
	if (userName == NULL || password == NULL){
		return command;
	}

	command.cmd = CMD_LOGIN;
	command.validArgs = true;
	sprintf(command.args,"%s\n%s",userName, password);

	return command;
}

/**Parses the arguments for CMD_ADD_COURSE.
 * If the arguments are valid course number and course name,
 * the field validArgs is set to true and the arguments are saved in the args field.
 * Otherwise, the field validArgs is set to false.
 *
 * @param cmd - the command
 * @param course_num, course_name - the command arguments
 * */
void parseAddCourse(Command* cmd, char* course_num, char* course_name){
	int num;
	if (course_num == NULL || course_name == NULL){
		cmd->validArgs = false;
		return;
	}
	if (!parseInt(course_num, &num) || !isValidCourseNum(num)){
		cmd->validArgs = false;
		return;
	}
	sprintf(cmd->args, "%s\n%s", course_num, course_name);
	cmd->validArgs = true;
}

/**Parses the arguments for CMD_RATE_COURSE.
 * If the arguments are valid course number, rating value and rating text,
 * the field validArgs is set to true and the arguments are saved in the args field.
 * Otherwise, the field validArgs is set to false.
 *
 * @param cmd - the command
 * @param course_num, rating_val, rating_txt - the command arguments
 * */
void parseRateCourse(Command* cmd, char* course_num, char* rating_val, char* rating_txt){
	int num;
	if (course_num == NULL || rating_val == NULL || rating_txt == NULL){
		cmd->validArgs = false;
		return;
	}
	if (!parseInt(course_num, &num) || !isValidCourseNum(num)){
		cmd->validArgs = false;
		return;
	}
	if (!parseInt(rating_val, &num) || !isValidRatingVal(num)){
		cmd->validArgs = false;
		return;
	}

	sprintf(cmd->args, "%s\n%s\n%s", course_num, rating_val, rating_txt);
	cmd->validArgs = true;
}

/**Parses the argument for CMD_GET_RATE.
 * If the argument is a valid course number,
 * the field validArgs is set to true and the argument is saved in the args field.
 * Otherwise, the field validArgs is set to false.
 *
 * @param cmd - the command
 * @param course_num - the command argument
 * */
void parseGetRate(Command* cmd, char* course_num){
	int num;
	if (course_num == NULL){
		cmd->validArgs = false;
		return;
	}
	if (!parseInt(course_num, &num) || !isValidCourseNum(num)){
		cmd->validArgs = false;
		return;
	}

	sprintf(cmd->args, "%s", course_num);
	cmd->validArgs = true;
}

/**Parses the arguments for CMD_ADD_COURSE.
 * If the arguments are valid course number and course name,
 * the field validArgs is set to true and the arguments are saved in the args field.
 * Otherwise, the field validArgs is set to false.
 *
 * @param cmd - the command
 * @param course_num, course_name - the command arguments
 * */
void parseBroadcast(Command* cmd, char* message){
	if (message == NULL){
		cmd->validArgs = false;
		return;
	}

	sprintf(cmd->args, "%s", message);
	cmd->validArgs = true;
}


/**Parses a specified line.
 *
 * If the line is a command which has arguments,
 * then the arguments are parsed and is saved in the field args,
 * and the field validArgs is set to true.
 *
 * In any other case then 'validArg' is set to false and the value 'args'
 * is undefined
 *
 * @return
 * A parsed line such that:
 *   cmd - contains the command type, if the line is invalid then this field is
 *         set to CMD_INVALID
 *   validArgs -  is set to true if the command contains arguments and the
 *                arguments are valid
 *   args      -  the arguments seperated by '\n'
 *                and validArgs is set to true
 */
Command parseLine(const char* str){
	Command command;
	char delimiter[] = " \t\r\n";
	char copy[MAX_LINE_LEN];
	char *token;


	command.cmd = CMD_INVALID;
	command.validArgs = false;
	if(str == NULL){
		return command;
	}
	strcpy(copy, str);
	token = strtok(copy, delimiter);	//get first token

	if(token == NULL){
		return command;
	}

	command.cmd = parseCommand(token);

	if (command.cmd == CMD_ADD_COURSE){
		char* num = strtok(NULL, delimiter);
		char* name = strtok(NULL, "\"");
		parseAddCourse(&command, num, name);
	}
	else if (command.cmd == CMD_RATE_COURSE){
		char* num = strtok(NULL, delimiter);
		char* val = strtok(NULL, delimiter);
		char* txt = strtok(NULL, "\"");
		parseRateCourse(&command, num, val, txt);
	}
	else if (command.cmd == CMD_GET_RATE){
		char* num = strtok(NULL, delimiter);
		parseGetRate(&command, num);
	}
	else if (command.cmd == CMD_BROADCAST){
		char* message = strtok(NULL, "\"");
		parseBroadcast(&command, message);
	}
	else {
		command.validArgs = true;
	}

	return command;

}
