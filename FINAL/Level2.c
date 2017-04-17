#include "helper.h"

int my_open(char *name){
//open file and return file decriptor
}

int close(int fd){
//closes the file opened that is stored in the fd
}

void read(int fd, void *buf, size_t count){
//reads count bytes from fd into buf
}

void write(int fd, const void *buf, size_t count){
//writes count bytes from buf into fd
}

void lseek(){
//offsets the fd by a certain byte
}

void cat(char *name){
//prints contents of file name to terminal
}

bool cp(char *name){
//copies the file name
}

bool mv(char *name,char *location){
//move the file to the directory
}

