#include "helper.h"











//Level 1 function

bool my_mkdir(char *name) {
	//make a dir


}

bool my_rmdir(char *name) {
	//remove dir
}

void my_ls(char *name) {
	//print directory contents 
	int i;
	MINODE *mip;
	DIR *dir;
	char buf[BLKSIZE], *cp;

	if (name[0] == '/')
	{
		dev = root->dev;
	}
	else
	{
		dev = running->cwd->dev;
	}

	i = getino(dev, name);
	if (!i)
	{
		printf("Error file not found \n\n\n");
		return;
	}
	mip = iget(dev, i);


	if (mip->ino == 0x8000)
	{
		printf("%s", basename(name));
	}
	else
	{
		for (int i = 0; i <= 11; i++)
		{
			if (mip->INODE.i_block[i])
			{
				get_block(dev, mip->INODE.i_block[i], buf);
				cp = buf;
				dir = (DIR *)buf;
				while (cp < &buf[BLKSIZE])
				{
					printf("%s ", dir->name);
					cp += dir->rec_len;
					dir = (DIR *)cp;
				}
			}
		}

	}
}

bool cd_root() {
	//change directory to root
}

bool cd_path(char *name) {
	//change directory to certain path
}

void my_pwd() {
	//print the full path name for the working directory 
}

bool my_creat(char *name) {
	//create a file with the given name
}

bool my_link() {
	//not sure what this actually does, maybe just links two files?
}

bool my_unlink() {
	//removes a link?
}

bool my_symlink() {
	//creates symbolic link
}

bool my_stat(char *name, ...) {
	//shows relevant information on the given inode
}

bool my_chmod(char *name, char permissions[]) {
	//changes the permissions of the file
}

bool my_touch(char *name) {
	//isnt this the same as creat?
}





//level 2 functions



int my_open(char *name) {
	//open file and return file decriptor
}

int close(int fd) {
	//closes the file opened that is stored in the fd
}

void read(int fd, void *buf, size_t count) {
	//reads count bytes from fd into buf
}

void write(int fd, const void *buf, size_t count) {
	//writes count bytes from buf into fd
}

void lseek() {
	//offsets the fd by a certain byte
}

void cat(char *name) {
	//prints contents of file name to terminal
}

bool cp(char *name) {
	//copies the file name
}

bool mv(char *name, char *location) {
	//move the file to the directory
}




// level 3 functions 





bool mount(char *device) {
	//mounts a device or disk, similar to root mounting
}

bool unmount(char *device) {
	//unmounts the device
}

bool check_Permissions(char *name, char permissisons[]) {
	//check permissions of file (executable, read only, etc)
}







//main functions





int main() {
	startMenu();
	return 0;
}

void startMenu() {
	char *command;
	printf("Welcome to our EXT2 file system\n\n");
	printf("Mounting root...\n\n");
	init();
	mountRoot("mydisk");
	printf("DONE! Please enter a command, listed below\n\n");
	printf("mkdir, rmdir, ls, cd, pwd, creat, link,  unlink, 		symlink, stat,  chmod, touch\n\n");
	printf("open, close, read, write, lseek, cat, cp, mv\n\n");
	printf("mount, unmount\n\n");
	while (1) {
		printf("Enter command:    ");
		//getline(&command, BLKSIZE, stdin);
		//execute_command(command);
		my_ls("/");
		system(pause);
		if (strcmp(command, "quit") == 0)
			break;
		else
			continue;
	}

	//simple function to display menu and accept command

}