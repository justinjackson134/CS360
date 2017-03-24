#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

char pathname[1024]; // Stores programs CWD

void getInput(char* line)
{
  do {                             // Do this while we have empty inputs
    printf("jjacks-shellsim:~");   // Print a shell message
    printf("%s$ ", pathname);      // Print the PWD$ after shell name
    fgets(line, 1024, stdin);      // Get the input line
    line[strcspn(line, "\n")] = 0; // Use strcspn to cut the newline off of the input
  } while(strcmp(line, "") == 0);  // Loop over(get new input) if no input given
}

void parseInput(char* line, char** commands)
{
  int i = 0;                                 // Int used to loop through piped commands
  commands[i] = strtok(line, "|");           // Tokenize first piped command
  do {                                       // Do until no more tokens
    if (commands[i][0] == ' ') {             // If the command starts with space, remove it!
      commands[i] = commands[i] + 1;         // Remove the first char, eliminates space after pipe
    }
    //DEBUG PRINT// printf("Command: %s\n", commands[i]);    // Echo commands per pipe
    i++;                                     // Increment counter
  } while (commands[i] = strtok(NULL, "|")); // If there are more commands/pipes, loop
}

void parseCommand(char* command, char** commandArgs)
{
  int i = 0;                                    // Int used to loop through command and args
  commandArgs[i] = strtok(command, " ");        // Tokenize the command
  do {                                          // Do until no more args
    /*
    if(i == 0)                                  // If on first element, it is the command
      printf("Command: %s\n", commandArgs[0]);  // Echo command name
    else                                        // Else, this is an arg
      printf("  Arg: %s\n", commandArgs[i]);    // Echo args per pipe
    */
    i++;                                        // Increment counter
  } while (commandArgs[i] = strtok(NULL, " ")); // If there are more commands/pipes, loop
}

int handleSimpleCommand(char* line)
{
  char* arg;                             // Used to store path arg if given
  
  if(line[0] == 'c' && line[1] == 'd') { // If line starts with cd, do the cd thing
    arg = strtok(line, " ");             // Tokenize first arg (gets rid of cd)
    arg = strtok(NULL, " ");             // Tokenize second arg (if not null, this is the destination
    
    if(arg == NULL) {                    // If arg == NULL, no argument was supplied, go home
      //DEBUG PRINT// printf("cd from %s to %s\n", pathname, getenv("HOME"));
      if(chdir(getenv("HOME"))) {        // Try to chdir, catch failures
	printf("CHDIR FAILURE\n");       // If we got into this if, chdir failed!
      }
    }
    else {                               // If arg != NULL, an argument was supplied, go there
      //DEBUG PRINT// printf("cd from %s to %s\n", pathname, arg);
      if(chdir(arg)) {                   // Try to chdir, catch failures
	printf("CHDIR FAILURE\n");       // If we got into this if, chdir failed!
      }
    }
    sprintf(pathname, "%s", "");         // Clear pathname or it just keeps expanding
    getcwd(pathname, 1024);              // Update pathname to programs CWD    
    return 1;                            // Return 1 to tell main that we did a simple cmd
  }				
  if(strcmp(line, "exit") == 0) {        // If line equals exit
    exit(1);                             // System exit
  }
  return 0;                              // Return 0 if we did not execute a simple cmd
}

int main(int argc, char* argv[], char* env[])
{           
  char line[1024];                              // Used to store the users input
  char *commands[1024];                         // Used to store commands tokenized by "|"
  char *commandArgs[1024];                      // Used to store individual commands and their args
  char pathToFunction[1024];                    // Used to store the path to users command
  char pathToFunctionUsrBin[1024];              // Used to store the path to USR/BIN
  int i = 0;                                    // Used to iterate through the commands array
  getcwd(pathname, 1024);                       // Initialize pathname
    
  while(1) {                                    // Loop until system exit call
    getInput(line);                             // Get input from command line
    //DEBUG PRINT// printf("echo-back: %s\n", line);            // Echo back whatever was entered by the user
    if(! handleSimpleCommand(line)) {           // Handle cd and exit (IF no simple cmd, goto advanced)
      parseInput(line, commands);               // Parse by pipes
      do {
	parseCommand(commands[i], commandArgs); // Parse by command and args

	int pid;                                // Declare PID
	int status = 0;                         // Declare Status
	pid = fork();                           // Fork the child

	if(pid < 0) {                           // If Child failed to spawn
	  printf("Failed to fork child\n");     // Inform user child failed to spawn
	  exit(1);                              // If forking failed, exit
	}
	if(pid) {                               // If we are the parent
	  pid = wait(&status);                  // Wait for child to die
	  //DEBUG PRINT// printf("PARENT: %d \tMY PARENT: %d\n", getpid(), getppid());
	  printf("CHILD (%d) DIED - Status: (%d)\n", pid, status);
	}
	else {                                  // Else we are the child
	  //DEBUG PRINT// printf("CHILD:  %d \tMY PARENT: %d\n", getpid(), getppid());
	  sprintf(pathToFunction, "/bin/%s", commandArgs[0]); // append the command to the bin directory
	  sprintf(pathToFunctionUsrBin, "/usr/bin/%s", commandArgs[0]); // append cmd to USR BIN
	  // This is to look in BIN
	  commandArgs[0] = pathToFunction;                    // Set commandArgs[0] equal to the filename
	  if(! execve(pathToFunction, commandArgs, env)) {    // Execute program, enter if if it fails
	    printf("Command: %s Not Found", commandArgs[0]);  // Return error message if command not found
	  }
	  // This is to look in the USR BIN
	  commandArgs[0] = pathToFunctionUsrBin;                    // Set commandArgs[0] equal to the filenam
	  if(! execve(pathToFunctionUsrBin, commandArgs, env)) {    // Execute program, enter if if it fails
	    printf("Command: %s Not Found", commandArgs[0]);        // Return error message if command not fou
	  }
	  
	  return 0;                                           // Return 0 to kill the child
	}
       	i++;                                    // Increment i (move to next command in the pipeline)
      } while (commands[i] != NULL);            // While we have more commands in our pipeline
    }
    i = 0;
  }
}
