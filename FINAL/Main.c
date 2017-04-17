
#include "Level1.h"
#include "Level2.h"
#include "Level3.h"





int main(){
	startMenu();
	return 0;
}

void startMenu(){
	char *command;
	printf("Welcome to our EXT2 file system\n\n");
	printf("Mounting root...\n\n");
	init();
	mountRoot("MyDisk");
	printf("DONE! Please enter a command, listed below\n\n");
	printf("mkdir, rmdir, ls, cd, pwd, creat, link,  unlink, 		symlink, stat,  chmod, touch\n\n");
	printf("open, close, read, write, lseek, cat, cp, mv\n\n");
	printf("mount, unmount\n\n");
	while (1) {
		printf("Enter command:    ");
		getline(&command, BLKSIZE, stdin);
		execute_command(command);
		my_ls();
		if (strcmp(command, "quit") == 0)
			break;
		else
			continue;
	}
		
//simple function to display menu and accept command

}


