#include "helper.h"


bool my_mkdir(char *name){
//make a dir


}

bool my_rmdir(char *name){
//remove dir
}

void my_ls(char *name){
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

bool cd_root(){
//change directory to root
}

bool cd_path(char *name){
//change directory to certain path
}

void my_pwd(){
//print the full path name for the working directory 
}

bool my_creat(char *name){
//create a file with the given name
}

bool my_link(){
//not sure what this actually does, maybe just links two files?
}

bool my_unlink(){
//removes a link?
}

bool my_symlink(){
//creates symbolic link
}

bool my_stat(char *name,...){
//shows relevant information on the given inode
}

bool my_chmod(char *name, char permissions[]){
//changes the permissions of the file
}

bool my_touch(char *name){
//isnt this the same as creat?
}


