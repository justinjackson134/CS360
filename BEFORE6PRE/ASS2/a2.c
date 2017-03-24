///////////////////////////////////////////////////////////////////
// PROGRAM: a2.c                                                  /
// DEFN:    This program contains myprintf() and its subfunctions /
//          Also contains Enque and Deque                         /
// AUTHOR:  Justin Jackson - 11437751                             /
// DATE:    1/26/2017                                             /
///////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/***** STRUCTS *****/
typedef struct node{
  struct node *next;
  char name[64];
  int priority;
}NODE;

/***** VARS *****/
NODE *readyQueue = 0; // an empty queue to start

int BASE16 = 16;
int BASE10 = 10;
int BASE8 = 8;
char *table = "0123456789ABCDEF";

/***** PRINT STRING *****/
int prints(char *s)
{
  while(*s)
  {
    putchar(*s++);
  }
}

/***** PRINT UNSIGNED INT *****/
typedef unsigned int u32;

int rpu(u32 x)
{
  char c;
  if (x)
  {
     c = table[x % BASE10];
     rpu(x / BASE10);
     putchar(c);
  }
} 

int printu(u32 x)
{
  if (x==0)
     putchar('0');
  else
     rpu(x);
}

/***** PRINT SIGNED INT *****/
int rpd(int x)
{
  char c;
  if (x)
  {
     c = table[x % BASE10];
     rpd(x / BASE10);
     putchar(c);
  }
} 

int printd(int x)
{
  if (x==0)
     putchar('0');
  else
     rpd(x);
}

/***** PRINT OCTAL INT *****/
int rpo(int x)
{
  char c;
  if (x)
  {
     c = table[x % BASE8];
     rpo(x / BASE8);
     putchar(c);
  }
} 

int printo(int x)
{
  putchar('0');
  if (x==0)
     putchar('0');
  else
     rpo(x);
}

/***** PRINT HEX INT *****/
int rpx(int x)
{
  char c;
  if (x)
  {
     c = table[x % BASE16];
     rpx(x / BASE16);
     putchar(c);
  }
} 

int printx(int x)
{
  putchar('0');
  putchar('x');
  if (x==0)
     putchar('0');
  else
     rpx(x);
}

/***** MYPRINTF *****/
int myprintf(char *fmt, ...)
{
  char *cp = fmt;
  int *ip = &fmt + 1;

  while(*cp)
  {
    if(*cp == '%')
    {
      *cp++;

      switch(*cp)
      {
      case 'c':
	putchar(*ip++);
	break;
      case 's':
	prints(*ip++);
	break;
      case 'u':
        printu(*ip++);
	break;
      case 'd':
	printd(*ip++);
	break;
      case 'o':
	printo(*ip++);
	break;
      case 'x':
	printx(*ip++);
	break;
      }
    }
    else
    {
      putchar(*cp);
      if(*cp == '\n')
      {
	putchar('\r');
      }
    }
    
    *cp++;
  }
  
  putchar('\n');
}

/***** enqueue *****/
int enqueue(NODE **queue, NODE *p) 
{
  NODE *curr = queue;
  //curr = curr->next;
  NODE *temp;

  int waitingToInsert = 1;
  
  //enters p into *queue by priority
  //myprintf("\nENQUEUE: %s, %d", p->name, p->priority);
  if(*queue == 0)
  {
    //myprintf("EMPTY QUEUE"); //myprintf("p: %d", p); myprintf("*queue: %d", *queue);
    *queue = p; //myprintf("p: %d", p); myprintf("*queue: %d", *queue);
  }
  else
  {
    while(waitingToInsert)
    {
      if((curr->next != NULL) && (curr->next->priority <= p->priority))
      {
	//myprintf("NOT AT END OF QUEUE - AND curr->priority <= p->priority");
	curr = curr->next;
      }
      else if(curr->next != NULL)
      {
	//myprintf("NOT AT END OF QUEUE - AND NOT AT END OF LIST");
	temp = curr->next;
	curr->next = p;
	p->next = temp;
	waitingToInsert = 0;
      }
      else if(curr->next == NULL)
      {
        //myprintf("AT END OF QUEUE");
        curr->next = p;
	waitingToInsert = 0;
      }
    }
  }
}

/***** dequeue *****/
NODE *dequeue(NODE **queue) 
{ 
  //delete the first node from *queue (if any) and return the NODE (pointer) with the highest priroity}
  //if empty, set *queue == 0

  NODE *curr = queue;
  NODE *returnMe = curr;
  
  *queue = curr->next;
  return returnMe;
}

/***** print priority queue *****/
NODE printPriorityQueue(NODE **queue)
{
  NODE *temp = queue;
  temp = temp->next;

  int atEndOfQueue = 0;

  myprintf("Begin Print Queue");
  if(*queue == 0)
  {
    myprintf("Queue Is Empty");
  }
  else
  {
    while(!atEndOfQueue)
    {
      if(temp != NULL)
      {
	myprintf("NODE-> %s, %d", temp->name, temp->priority);	
	temp = temp->next;
      }
      else if(temp == NULL)
      {
        myprintf("End Of Queue\n");
	atEndOfQueue = 1;
      }
    }
  }
}

/***** MAIN *****/
int main(int argc, char *argv[ ], char *env[ ])
{
  
  NODE *p;

  myprintf("\nCS360 PROGRAMMING ASSIGNMENT #2 - JUSTIN JACKSON\n------------------------------------------------------");
  myprintf("\nARGC:\n------------------------------------------------------\n %d", argc);
  myprintf("\nARGV:\n------------------------------------------------------");
  while(*argv)
  {
    myprintf("%s",*argv++);
  }  
  myprintf("\nENVR:\n------------------------------------------------------");
  while(*env)
  {
    myprintf("%s",*env++);
  }

  myprintf("\nPQUE:\n------------------------------------------------------");
  int i = 0;
  while(i<15)
  {
    p = malloc(sizeof(struct node)); 
    sprintf(p->name, "node%d", i);             // NOTE: i=0,1,2,....
    //myprintf("Name: %s", p->name);           // DEBUG prints name
    p->priority = rand() % 10;                 // value = 0-9
    //myprintf("Priority: %d", p->priority);   // DEBUG prints priority
    enqueue(&readyQueue, p);                   // enqueue
    i++;
  }
  printPriorityQueue(&readyQueue);           // show the readyQueue
}

//myprintf("TESTING myprintf() CHAR: %c, STRN: %s, UNSN: %u, DECI: %d, OCTA: %o, HEX : %x.", 'A', "TestString", 42, 42, 42, 42);
