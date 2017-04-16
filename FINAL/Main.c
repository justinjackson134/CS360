#include "helper.h"





int main(){
	startMenu();
	return 0;
}

void startMenu(){
	char *command;
	printf("Welcome to our EXT2 file system\n\n");
	printf("Mounting root...\n\n");
	init();
	mountRoot();
	printf("DONE! Please enter a command, listed below\n\n");
	printf("mkdir, rmdir, ls, cd, pwd, creat, link,  unlink, 		symlink, stat,  chmod, touch\n\n");
	printf("open, close, read, write, lseek, cat, cp, mv\n\n");
	printf("mount, unmount\n\n");
	while (1) {
		printf("Enter command:    ");
		getline(&command, BLKSIZE, stdin);
		execute_command(command);
		if (strcmp(command, "quit") == 0)
			break;
		else
			continue;
	}
		
//simple function to display menu and accept command

}


