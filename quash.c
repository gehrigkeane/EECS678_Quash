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

static struct job all_jobs[MAX_NUM_JOBS];

static int num_jobs;

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
 * Helper Functions 
 **************************************************************************/
bool is_running() {
	return running;
}

void terminate() {
	running = false;
}

void mask_signal(int signal)
{
	printf("\n");
}

void unmask_signal(int signal)
{
	exit(0); 
} 
/**************************************************************************
 * String Manipulation Functions 
 **************************************************************************/

/**
	* Parse Raw Command string
	*
	* @param cmd command struct
	* @param in instream
	* @return bool successful parse
	*/
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

		char* token = malloc( sizeof(char*) * MAX_COMMAND_ARGLEN );

		token = strtok (cmd->cmdstr," ");
		while (token != NULL)
		{
			//debug print - printf ("%d: %s\n", (int)cmd->toklen, token);
			cmd->tok[cmd->toklen] = token;
			cmd->toklen++;
			token = strtok (NULL, " ");
		}

		free(token);
		////////////////////////////////////////////////////////////////////////////////
		// Remove NULL token from end
		////////////////////////////////////////////////////////////////////////////////
		cmd->tok[cmd->toklen] = '\0';

		return true;
	}
	else
		return false;
}

/**
	* Print Current Working Directory before shell commands
	*
	* @return void
	*/
void print_init() {
	////////////////////////////////////////////////////////////////////////////////
	// Print Current Working Dir before each command
	////////////////////////////////////////////////////////////////////////////////
	char cwd[MAX_COMMAND_LENGTH];		//cwd arg - print before each shell command
	if ( getcwd(cwd, sizeof(cwd)) )
		printf("%s $ ", cwd);
}

/**************************************************************************
 * Shell Fuctionality 
 **************************************************************************/

/**
	* CD Implementation
	*
	* @param cmd command struct
	* @return void
	* Note: chdir will make new dir's if they don't exist
	*/
void cd(command_t* cmd) 
{
	if ( cmd->toklen < 2 ) {
		if ( chdir(getenv("HOME")) )
			printf("cd: %s: Cannot navigate to $HOME\n", getenv("HOME"));
	}
	else if ( cmd->toklen > 2 )
		puts("Too many arguments");
	else { 
		if ( chdir(cmd->tok[1]) )
			printf("cd: %s: No such file or directory\n", cmd->tok[1]); 
	}
}

/**
	* Echo Implementation
	*
	* @param cmd command struct
	* @return void
	*/
void echo(command_t* cmd) 
{
	if ( cmd->toklen == 2 ) {
		if ( !strcmp(cmd->tok[1], "$HOME") )
			puts(getenv("HOME"));
		else if ( !strcmp(cmd->tok[1], "$PATH") )
			puts(getenv("PATH"));
		else
			puts(cmd->tok[1]);
	}
	else if ( cmd->toklen == 1 ) {
		puts(getenv("HOME"));
	}
	else{
		int i = 1;
		for ( ; i < cmd->toklen; i++ )
			printf("%s ", cmd->tok[i]);
		puts("");
	}
}

/**
	* Displays all currently running jobs
	* @return void
	*/
void jobs(command_t* cmd) {
	if (cmd->tok[1] == NULL) {
		int i = 0;
		for (; i < num_jobs; i++) {
			if(all_jobs[i].status) {
				printf("[%d] %d %s \n", all_jobs[i].jid, all_jobs[i].pid, all_jobs[i].cmdstr); 
			}
		}
	}
	else {
		printf("jobs: Unknown command \"%s\"\n", cmd->tok[1]);
	}
}

/**
	* Set Implementation
	*
	* Assigns the specified environment variable (HOME or PATH),
	* or displays an error for user mistakes.
	*
	* @param cmd command struct
	* @return void
	*/
void set(command_t* cmd) {
	if (cmd->tok[1] == NULL) {
		printf("set: No command given\n");
	}
	else {
		// Get the environment variable and directory
		// (Delimited by '=')
		char* env = strtok(cmd->tok[1], "=");
		char* dir = strtok(NULL, "=");

		if (env == NULL || dir == NULL) {
			printf("set: Incorrect syntax. Possible Usages:\n");
			printf("\tset PATH=/directory/to/use/for/path\n");
			printf("\tset HOME=/directory/to/use/for/home\n");
		}
		else if (!strcmp(env, "PATH") || !strcmp(env, "HOME")) {
			// Set the environment variable
			setenv(env, dir, 1);
		}
		else {
			printf("set: available only for PATH or HOME environment variables\n");
		}
	}
}

/**************************************************************************
 * Execution Functions 
 **************************************************************************/

/**
	* Command Decision Structure
	*
	* @param cmd command struct
	* @param envp environment variables
	* @return RETURN_CODE
	*/
int exec_command(command_t* cmd, char* envp[])
{
	////////////////////////////////////////////////////////////////////////////////
	// Command Flag Initializations
	////////////////////////////////////////////////////////////////////////////////
	bool b_bool = false;	//background
	bool i_bool = false;	//input redirection
	bool o_bool = false;	//output redirection
	bool p_bool = false;	//pipe

	////////////////////////////////////////////////////////////////////////////////
	// Walk command tokens and flip flags
	////////////////////////////////////////////////////////////////////////////////
	int i = 1;
	for (; i < cmd->toklen; i++) {
		if ( !strcmp(cmd->tok[i], "&") )
			b_bool = true;
		else if ( !strcmp(cmd->tok[i], "<") )
			i_bool = true;
		else if ( !strcmp(cmd->tok[i], ">") )
			o_bool = true;
		else if ( !strcmp(cmd->tok[i], "|") )
			p_bool = true;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Execute designated command
	////////////////////////////////////////////////////////////////////////////////
	int RETURN_CODE = 0;
	// we have access to numArgs here and this will be portable
	if ( b_bool ) {
		cmd->tok[cmd->toklen - 1] = '\0'; // remove & token
		cmd->toklen--;
		//execute background command
	} 
	else if ( i_bool ) {
		RETURN_CODE = exec_redir_command(cmd, true, envp);
	}
	else if ( o_bool ) {
		RETURN_CODE = exec_redir_command(cmd, false, envp);
	}
	else if ( p_bool ) {
		// execute pipe command
	}
	else {
		RETURN_CODE = exec_basic_command(cmd, envp);
	}
	return RETURN_CODE;
}

/**
	* Executes any command that can be handled with execvpe (ergo free of |, <, >, or &)
	*
	* @param cmd command struct
	* @param envp environment variables
	* @return RETURN_CODE
	*/
int exec_basic_command(command_t* cmd, char* envp[])
{
	////////////////////////////////////////////////////////////////////////////////
	// Mask Inturrept Signals and Initialize Variables
	////////////////////////////////////////////////////////////////////////////////
	pid_t p;
	int wait_status;
	signal(SIGINT, mask_signal);  

	////////////////////////////////////////////////////////////////////////////////
	// Fork And Verify Process
	////////////////////////////////////////////////////////////////////////////////
	p = fork();
	if (p < 0) {
		fprintf(stderr, "Error forking basic command. Error:%d\n", errno);
		exit(EXIT_FAILURE);
	}

	////////////////////////////////////////////////////////////////////////////////
	// Parent
	////////////////////////////////////////////////////////////////////////////////
	if (p != 0) {
		if ( waitpid(p, &wait_status, 0) < 0 ) {
			signal(SIGINT, unmask_signal);
			fprintf(stderr, "Error with basic command's child  %d. ERRNO\"%d\"\n", p, errno);
			return EXIT_FAILURE;
		}
		if ( WIFEXITED(wait_status) && WEXITSTATUS(wait_status) == EXIT_FAILURE )
			return EXIT_FAILURE;

		signal(SIGINT, unmask_signal);
		return EXIT_SUCCESS;
	}
	
	////////////////////////////////////////////////////////////////////////////////
	// Child
	////////////////////////////////////////////////////////////////////////////////
	else {
		if ( execvpe(cmd->tok[0], cmd->tok, envp) < 0  && errno == 2 ) {
			fprintf(stderr, "Command: \"%s\" not found.\n", cmd->tok[0]);
			exit(EXIT_FAILURE);
		}
		else {
			fprintf(stderr, "Error executing %s. ERRNO\"%d\"\n", cmd->tok[0], errno);
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS);
	}
}

/**
	* Executes any command with an I/O redirection present 
	*
	* @param cmd command struct
	* @param io true is stdin, false is stdout
	* @param envp environment variables
	* @return RETURN_CODE
	*/
int exec_redir_command(command_t* cmd, bool io, char* envp[])
{
	////////////////////////////////////////////////////////////////////////////////
	// Mask Inturrept Signals and Initialize Variables
	////////////////////////////////////////////////////////////////////////////////
	pid_t p;
	int wait_status;
	int file_desc;
	signal(SIGINT, mask_signal);  

	////////////////////////////////////////////////////////////////////////////////
	// Fork And Verify Process
	////////////////////////////////////////////////////////////////////////////////
	p = fork();
	if (p < 0) {
		fprintf(stderr, "Error forking redir command. ERRNO\"%d\"\n", errno);
		exit(EXIT_FAILURE);
	}

	////////////////////////////////////////////////////////////////////////////////
	// Parent
	////////////////////////////////////////////////////////////////////////////////
	if (p != 0) {
		if (waitpid(p, &wait_status, 0) == -1) {
			fprintf(stderr, "Error with redir command's child  %d. ERRNO\"%d\"\n", p, errno);
			return EXIT_FAILURE;
		}
		if ( WIFEXITED(wait_status) && WEXITSTATUS(wait_status) == EXIT_FAILURE )
			return EXIT_FAILURE;

		signal(SIGINT, unmask_signal);
		return EXIT_SUCCESS;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Child
	////////////////////////////////////////////////////////////////////////////////
	else {
		////////////////////////////////////////////////////////////////////////////////
		// Initialize and Verify File Descriptor
		////////////////////////////////////////////////////////////////////////////////
		if (io)
			file_desc = open(cmd->tok[cmd->toklen - 1], O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		else
			file_desc = open(cmd->tok[cmd->toklen - 1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

		if (file_desc < 0) {
			fprintf(stderr, "\nError opening %s. ERRNO\"%d\"\n", cmd->tok[cmd->toklen - 1], errno);
			exit(EXIT_FAILURE);
		}

		////////////////////////////////////////////////////////////////////////////////
		// Redirect I/O Streams
		////////////////////////////////////////////////////////////////////////////////
		if (io) {
			if (dup2(file_desc, STDIN_FILENO) < 0) {
				fprintf(stderr, "\nError redirecting STDIN to %s. ERRNO\"%d\"\n", cmd->tok[cmd->toklen - 1], errno);
				exit(EXIT_FAILURE);
			}
		}
		else {
			if (dup2(file_desc, STDOUT_FILENO) < 0) {
				fprintf(stderr, "\nError redirecting STDOUT to %s. ERRNO\"%d\"\n", cmd->tok[cmd->toklen - 1], errno);
				exit(EXIT_FAILURE);
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		// Execute Command - remove last two redirection arguments
		////////////////////////////////////////////////////////////////////////////////
		close(file_desc);
		cmd->tok[cmd->toklen - 1] = NULL;
		cmd->tok[cmd->toklen - 2] = NULL;
		cmd->toklen = cmd->toklen - 2;

		if ( execvpe(cmd->tok[0], cmd->tok, envp) < 0  && errno == 2 ) {
			fprintf(stderr, "Command: \"%s\" not found.\n", cmd->tok[0]);
			exit(EXIT_FAILURE);
		}
		else {
			fprintf(stderr, "Error executing %s. ERRNO\"%d\"\n", cmd->tok[0], errno);
			exit(EXIT_FAILURE);
		}
		signal(SIGINT, unmask_signal);
		exit(EXIT_SUCCESS);
	}
}

/**************************************************************************
 * MAIN
 **************************************************************************/

/**
	* Quash entry point
	*
	* @param argc argument count from the command line
	* @param argv argument vector from the command line
	* @return program exit status
	*/
int main(int argc, char** argv, char** envp) { 

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
			jobs(&cmd);
		}
		else if (!strcmp(cmd.tok[0], "kill")) {
			//TODO: IMPLEMENT KILL FUNCTION HERE
			printf("IMPLEMENT KILL FUNCTION\n");
		}
		else if (!strcmp(cmd.tok[0], "set")) {
			set(&cmd);
		}
		else {
			//TODO: IMPLEMENT EXECUTE COMMAND FUNCTION
			/*int i = 0;
			puts("Struct Token String\n");
			for (;i <= cmd.toklen; i++) {
				printf ("%d: %s\n", i, cmd.tok[i]);
			}*/
			exec_command(&cmd, envp);
		}

		print_init();
	}

	return EXIT_SUCCESS;
}
