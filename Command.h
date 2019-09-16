/*
 * Command.h
 *
 *  Created on: 24 Mar 2019
 *      Author: lital
 */

#ifndef COMMAND_H_
#define COMMAND_H_

#include <stdbool.h>

#define MAX_LINE_LEN 4096
#define MAX_USER_NAME_LEN 15
#define MAX_PASS_LEN 15
#define MAX_DATA_LEN 2048

//Used to represent a command type
typedef enum Command_Type{
	CMD_LOGIN,
	CMD_LIST_OF_COURSES,
	CMD_ADD_COURSE,
	CMD_RATE_COURSE,
	CMD_GET_RATE,
	CMD_BROADCAST,
	CMD_QUIT,
	CMD_INVALID
} Command_Type;

//A new type that is used to encapsulate a parsed line
typedef struct Command{
	Command_Type cmd;
	bool validArgs; //is set to true if the line contains valid arguments
	char args[MAX_DATA_LEN+1];
} Command;

/**
 * Parses a specified line.
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
Command parseLine(const char* str);

/**Parses the arguments for CMD_LOGIN.
 * If the arguments are valid user name and password,
 * the field validArgs is set to true and the arguments are saved in the args field.
 * Otherwise, the field validArgs is set to false.
 *
 * @param cmd - the command
 * @param user_line, pass_line - the command arguments
 * */
Command parseLogin(const char* user_line, char* pass_line);



#endif /* COMMAND_H_ */
