#include "helper.h"


bool mkdir(char *name,...){
//make a dir
}

bool rmdir(char *name,...){
//remove dir
}

void ls(){
//print directory contents 
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


