/**
	* @file quash.h
	*
	* Quash essential functions and structures.
	*/

#ifndef QUASH_H
#define QUASH_H

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/**
	* Specify the maximum number of characters accepted by the command string
	*/
#define MAX_COMMAND_LENGTH (1024)
#define MAX_COMMAND_ARGLEN (32)

#define MAX_NUM_JOBS (100)

/**
	* Holds information about a command.
	*/
typedef struct command_t {
		char** tok;							//tokenized command
		char cmdstr[MAX_COMMAND_LENGTH];	///< character buffer to store the
											///< command string. You may want
											///< to modify this to accept
											///< arbitrarily long strings for
											///< robustness.
		size_t cmdlen;						///< length of the cmdstr character buffer
		size_t toklen;
		// Extend with more fields if needed
} command_t;

/**
	* Holds information about a running process (job).
	*/
typedef struct job {
	char* cmdstr;							// The command issued for this process
	bool status;							// Status for this process (running or not)
	int pid;								// Process ID #
	int jid;								// Job ID #
} job;

/**
	* Query if quash should accept more input or not.
	*
	* @return True if Quash should accept more input and false otherwise
	*/
bool is_running();

/**
	* Causes the execution loop to end.
	*/
void terminate();

/**
	*  Read in a command and setup the #command_t struct. Also perform some minor
	*  modifications to the string to remove trailing newline characters.
	*
	*  @param cmd - a command_t structure. The #command_t.cmdstr and
	*               #command_t.cmdlen fields will be modified
	*  @param in - an open file ready for reading
	*  @return True if able to fill #command_t.cmdstr and false otherwise
	*/
bool get_command(command_t* cmd, FILE* in);

//Signal Masking definition
sigset_t mask;

#endif // QUASH_H
