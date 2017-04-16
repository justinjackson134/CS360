#include "helper.h"


bool mkdir(char *name,...){
//make a dir


}

bool rmdir(char *name,...){
//remove dir
}

void ls(char *name){
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
					dir = (DIR *)cp
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

void pwd(){
//print the full path name for the working directory 
}

bool creat(char *name){
//create a file with the given name
}

bool link(){
//not sure what this actually does, maybe just links two files?
}

bool unlink(){
//removes a link?
}

bool symlink(){
//creates symbolic link
}

bool stat(char *name,...){
//shows relevant information on the given inode
}

bool chmod(char *name, char permissions[]){
//changes the permissions of the file
}

bool touch(char *name){
//isnt this the same as creat?
}


