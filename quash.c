/**
 * @file quash.c
 *
 * Quash's main file
 */

/**************************************************************************
 * Included Files
 **************************************************************************/ 
#include "quash.h"	// Putting this above the other includes allows us to ensure
					// this file's headder's #include statements are self
					// contained.

/**************************************************************************
 * Private Variables
 **************************************************************************/
/**
 * Keep track of whether Quash should request another command or not.
 */
// NOTE: "static" causes the "running" variable to only be declared in this
// compilation unit (this file and all files that include it). This is similar
// to private in other languages.
static bool running;

/**************************************************************************
 * Private Functions 
 **************************************************************************/
/**
 * Start the main loop by setting the running flag to true
 */
static void start() {
	running = true;
}

/**************************************************************************
 * Public Functions 
 **************************************************************************/
bool is_running() {
	return running;
}

void terminate() {
	running = false;
}

bool get_command(command_t* cmd, FILE* in) {
	if (fgets(cmd->cmdstr, MAX_COMMAND_LENGTH, in) != NULL) {
		size_t len = strlen(cmd->cmdstr);
		char last_char = cmd->cmdstr[len - 1];

		if (last_char == '\n' || last_char == '\r') {
			// Remove trailing new line character.
			cmd->cmdstr[len - 1] = '\0';
			cmd->cmdlen = len - 1;
		}
		else
			cmd->cmdlen = len;

		////////////////////////////////////////////////////////////////////////////////
		// Empty Command return true - MUST BE HANDLED
		////////////////////////////////////////////////////////////////////////////////
		if (!(int)cmd->cmdlen)
			return true;

		////////////////////////////////////////////////////////////////////////////////
		// Tokenize command arguments
		////////////////////////////////////////////////////////////////////////////////
		cmd->tok = malloc( sizeof(char*) * MAX_COMMAND_ARGLEN );
		cmd->toklen = 0;

		char * temp = cmd->cmdstr;
		temp = strtok (cmd->cmdstr," ");
		while (temp != NULL)
		{
			//debug print - printf ("%d: %s\n", (int)cmd->toklen, temp);
			cmd->tok[cmd->toklen] = temp;
			cmd->toklen++;
			temp = strtok (NULL, " ");
		}

		////////////////////////////////////////////////////////////////////////////////
		// Remove NULL token from end
		////////////////////////////////////////////////////////////////////////////////
		cmd->toklen--;

		return true;
	}
	else
		return false;
}

void print_init() {
	////////////////////////////////////////////////////////////////////////////////
	// Print Current Working Dir before each command
	////////////////////////////////////////////////////////////////////////////////
	char cwd[MAX_COMMAND_LENGTH];		//cwd arg - print before each shell command
	if ( getcwd(cwd, sizeof(cwd)) )
		printf("%s $ ", cwd);
}


/**
	* Echo Implementation
	*
	* @param cmd command struct
	* @return void
 */
void echo(command_t* cmd) 
{
	if ( cmd->toklen != 1 || strcmp(cmd->tok[1], "$HOME") )
		puts(getenv("HOME"));
	else if ( strcmp(cmd->tok[1], "$PATH") )
		puts(getenv("PATH"));
	else
		puts(cmd->tok[1]);
}


/**
	* CD Implementation
	*
	* @param cmd command struct
	* @return void
	* Note: chdir will make new dir's if they don't exist
 */
void cd(command_t* cmd) 
{
	if ( cmd->toklen < 1 ) {
		if ( chdir(getenv("HOME")) )
			printf("cd: %s: Cannot navigate to $HOME\n", getenv("HOME"));
	}
	else if ( cmd->toklen > 1 )
		puts("Too many arguments");
	else { 
		if ( chdir(cmd->tok[1]) )
			printf("cd: %s: No such file or directory\n", cmd->tok[1]); 
	}
}

/**
	* Quash entry point
	*
	* @param argc argument count from the command line
	* @param argv argument vector from the command line
	* @return program exit status
 */
int main(int argc, char** argv) { 

	////////////////////////////////////////////////////////////////////////////////
	// Init Signal Masking
	////////////////////////////////////////////////////////////////////////////////
	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);

	////////////////////////////////////////////////////////////////////////////////
	// Input stems from FILE
	////////////////////////////////////////////////////////////////////////////////
	if ( !isatty( (fileno(stdin) ) ) ) {
		//Detect quash execution with redirected input
		//We'll deal with this later
		return 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Args
	////////////////////////////////////////////////////////////////////////////////
	command_t cmd;						//< Command holder argument

	start();
	
	puts("Welcome to Quash!");
	puts("Type \"exit\" or \"quit\" to leave this shell");
	print_init();

	// Main execution loop
	while (is_running() && get_command(&cmd, stdin)) {

		// The commands should be parsed, then executed.
		if (!strcmp(cmd.cmdstr, "exit") || !strcmp(cmd.cmdstr, "quit")) {
			terminate(); // Exit Quash
		}
		else if (!cmd.cmdlen) {
			print_init();
			continue;
		}
		else if (strcmp(cmd.tok[0], "cd") == 0) {
			cd(&cmd);
		}
		else if (strcmp(cmd.tok[0], "echo") == 0) {
			echo(&cmd);
		}
		else if (!strcmp(cmd.tok[0], "jobs")) {
			//TODO: IMPLEMENT JOBS FUNCTION HERE
			printf("IMPLEMENT JOBS FUNCTION\n");
		}
		else if (!strcmp(cmd.tok[0], "kill")) {
			//TODO: IMPLEMENT KILL FUNCTION HERE
			printf("IMPLEMENT KILL FUNCTION\n");
		}
		else if (!strcmp(cmd.tok[0], "set")) {
			//TODO: IMPLEMENT SET FUNCTION HERE
			printf("IMPLEMENT SET FUNCTION\n");
		}
		else {
			//TODO: IMPLEMENT EXECUTE COMMAND FUNCTION
			int i = 0;
			puts("Struct Token String\n");
			for (;i <= cmd.toklen; i++) {
				printf ("%d: %s\n", i, cmd.tok[i]);
			}
		}

		print_init();
	}

	return EXIT_SUCCESS;
}
