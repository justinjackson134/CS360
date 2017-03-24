/******************* l1.c ********************************************/
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

main()
{
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

int printlist(char *name, NODE *p)
{
   printf("%s = ", name);
   while(p){
     printf("[%s %d]->", p->name, p->id);
      p = p->next;
  }
  printf("NULL\n");
}