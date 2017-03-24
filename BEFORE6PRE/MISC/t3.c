/******* t3.c file *******/



#include <stdio.h>   
#include <stdlib.h>  
#include <string.h>

#define N 10

typedef struct node{
  struct node *next;
  char name[64];
  int  id;
}NODE;

char name[64];
NODE *mylist,       node[N]; // all in the bss section of run-time image




int *FP; // a global pointer      
main(int argc, char *argv[], char *env[])    
{ 
	int a,b,c;        
	printf("enter main\n");        
	a=1; b=2;
	c=3;        
	A(a,b);
	printf("exit main\n");    




 int i; 
  NODE *p;

  for (i=0; i < N; i++){
     p = &node[i];
     sprintf(name, "%s%d", "node",i); // node0, node1, node2 etc
     strcpy(p->name, name);
     p->id = i;
     p->next = p+1;        // node[i].next = &node[i+1];
  }
  node[N-1].next = 0;
  mylist = &node[0];

  printlist("mylist", mylist);







}    
int A(int x, int y)    
{  
	int d,e,f;        
	printf("enter A\n");        
	d=4; e=5; f=6;
	B(d,e);        
	printf("exit A\n");    
}    
int B(int x, int y) 
{  
	int u,v,w;        
	printf("enter B\n");        
	u=7; v=8; w=9;        
	asm("movl %ebp, FP"); // set FP=CPU's %ebp register        // Write C code to DO (1)-(3) AS SPECIFIED BELOW        
	printf("exit B\n");    
}





int printlist(char *name, NODE *p)
{
   printf("%s = ", name);
   while(p){
     printf("[%s %d]->", p->name, p->id);
      p = p->next;
  }
  printf("NULL\n");
}