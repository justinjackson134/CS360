// Includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

// Define a C struct for the NODE type
typedef struct node{
  struct node *childPtr, *siblingPtr, *parentPtr;
  char name[64]; // 64 chars : name string of the node;
  char nodeType; // char     : node type: 'D' or 'F'
}NODE;

//Global Variables:
NODE *root, *cwd, *curr;            /* root and CWD pointers */
char line[128];                     /* user input line */
char command[16], pathname[64];     /* user inputs */
char dir_name[64], base_name[64];   /* string holders */
int errorOccurred;                   /* used in mkdir */
//(Others as needed)

//CMD FINDER
char *cmd[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "creat", "rm",
               "quit", "help", "?", "menu", "reload", "save", 0};

int findCmd(char *command)
{ 
  int i = 0;
  if (command == NULL)
  {
    return -1;
  }
  while(cmd[i])
  {
    //printf("%s == %s\n", command, cmd[i]); 
    if (strcmp(command, cmd[i])==0)
    {
      //printf("WE FOUND IT AT: %d\n", i);
      return i;
    }
    i++;
  }
  return -1;
}

// Initialize root node and prompt to reload existing file
void initialize(void)
{
  root = malloc(sizeof(struct node));  
  sprintf(root->name, "root");
  //root->siblingPtr = root;
  //root->parentPtr = root;

  cwd = root;
}

void searchPath()
{
  char *token;
  
  token = strtok(dir_name, "/");
  while(token)
  {    
    printf("Checking if %s exists\n", token);
    curr = curr->childPtr;
    
    while(curr != NULL)
    {
      printf("Checking Sibling...\n");
      if(strcmp(curr->name, token) == 0)
      {
	printf("Found: %s\n", token);
	if(curr->nodeType == 'F')
	{
	  printf("ERROR: Directory is of type, FILE\n");
	  errorOccurred = 1;
	}
	break;
      }
      else
      {
	printf("Wrong Sibling: %s\n", curr->name);
	curr = curr->siblingPtr;
      }
    }

    if(curr == NULL)
    {
      printf("ERROR: Directory does not exist (OUT OF SIBLINGS)\n");
      errorOccurred = 1;
      break;
    }
    token = strtok(NULL, "/");
  }
}

void mkdir()
{
  NODE *dir, *tempNode;
  char *token;
  char *lastToken;
  char *temp;
  int isAbsolute = 0;
  
  // Reset dirname and basename every loop
  sprintf(dir_name, "%s", "");
  sprintf(base_name, "%s", "");

  // Reset error condition
  errorOccurred = 0;
  
  if(pathname[0] == '/')
  {
    isAbsolute = 1;
  }

  token = strtok(pathname, "/");
  while(token)
  {
    lastToken = token;
    token = strtok(NULL, "/");

    if(token)
    {
      //append last to dir_name
      strcat(dir_name, lastToken);
      strcat(dir_name, "/");
    }
    else
    {
      //lastToken = basename
      sprintf(base_name, "%s", lastToken);
    }
  }
 
  // IF this is an Absolute pathname, append a / to the front
  if(isAbsolute)
  {
    temp = strdup(dir_name);
    strcpy(dir_name, "/");
    strcat(dir_name, temp);
  }
   
  // Search for dir_name node
  if(isAbsolute)
  {
    printf("Search mode is absolute\n");
    curr = root;
  }
  else
  {
    printf("Search mode is not absolute\n");
    curr = cwd;
  }
    
  searchPath();

  if(errorOccurred) // If there was an error during search, dont make a node.
  {
    return;
  }

  tempNode = curr->childPtr;
  while (tempNode != NULL)
  {
    if(strcmp(tempNode->name, base_name) == 0)
    {
      printf("ERROR: There is already a directory or file by this name\n");
      return;
    }
    tempNode = tempNode->siblingPtr;
  }  
  
  // ASSUME WE ARE IN THE RIGHT DIRECTORY //create new dir
  dir = malloc(sizeof(struct node));
  sprintf(dir->name, "%s", base_name);
  dir->nodeType = 'D';
  dir->parentPtr = curr;
  dir->siblingPtr = NULL;
  dir->childPtr = NULL;
  printf("dir->name: %s\ndir->nodeType:", dir->name); putchar(dir->nodeType); putchar('\n');
  printf("dir->parent: %s", dir->parentPtr->name); putchar('\n');
  if(curr->childPtr == NULL)
  {
    printf("There is no child, curr->childPtr = dir\n");
    curr->childPtr = dir;
  }
  else
  {    
    printf("There is a child, curr = curr->childPtr\n");
    curr = curr->childPtr;
    
    while(curr->siblingPtr != NULL)
    {
      printf("Curr has a sibling, curr = curr->siblingPtr\n");
      curr = curr->siblingPtr;
    }

    printf("Curr has no more siblings, curr->siblingPtr = dir\n");
    curr->siblingPtr = dir;
  }
}
void rmdir()
{
  NODE *tempNode, *lastTempNode;
  char *token;
  char *lastToken;
  char *temp;
  int isAbsolute = 0;
  
  // Reset dirname and basename every loop
  sprintf(dir_name, "%s", "");
  sprintf(base_name, "%s", "");

  // Reset error condition
  errorOccurred = 0;
  
  if(pathname[0] == '/')
  {
    isAbsolute = 1;
  }

  token = strtok(pathname, "/");
  while(token)
  {
    lastToken = token;
    token = strtok(NULL, "/");

    if(token)
    {
      //append last to dir_name
      strcat(dir_name, lastToken);
      strcat(dir_name, "/");
    }
    else
    {
      //lastToken = basename
      sprintf(base_name, "%s", lastToken);
    }
  }
 
  // IF this is an Absolute pathname, append a / to the front
  if(isAbsolute)
  {
    temp = strdup(dir_name);
    strcpy(dir_name, "/");
    strcat(dir_name, temp);
  }
   
  // Search for dir_name node
  if(isAbsolute)
  {
    printf("Search mode is absolute\n");
    curr = root;
  }
  else
  {
    printf("Search mode is not absolute\n");
    curr = cwd;
  }
    
  searchPath();

  if(errorOccurred) // If there was an error during search, dont delete a node.
  {
    return;
  }

  tempNode = curr->childPtr;
  if(tempNode->childPtr != NULL)
  {
    printf("ERROR: Directory to delete is not empty\n");
    return;   
  }

  printf("REMOVING: %s\n", pathname);

  //If here, delete node
  while(tempNode->siblingPtr != NULL)
  {
    if(strcmp(tempNode->name, pathname) == 0)
    {
      if(tempNode->siblingPtr != NULL)
      {
	printf("THERE IS A SIBLING. DONT ORPHAN THEM.\n");
	if(lastTempNode != NULL)
	{
	  printf("MIDDLE DELETION\n");
	  lastTempNode->siblingPtr = tempNode->siblingPtr;
	}
	else
	{
	  printf("FIRST DELETION\n");
	  curr->childPtr = tempNode->siblingPtr;
	}
	if(lastTempNode != NULL)
	{
	  // curr->childPtr = lastTempNode;
	}
	return;
      }
      else
      {
	printf("NO SIBLINGS. EXECUTE THEM.\n");
	curr->childPtr = NULL ;
	return;
      }
    }
    else
    {
      lastTempNode = tempNode;
      tempNode = tempNode->siblingPtr;
    }
  } 
}
void ls()
{
  NODE *curr;
  curr = cwd->childPtr;
  while (curr != NULL)
  {
    printf("%c\t%s\n", curr->nodeType, curr->name);
    curr = curr->siblingPtr;
  }
}
void cd()
{
  curr = cwd;
  
  if(strcmp(pathname, "") == 0)
  {
    printf("CD NO PATH\n");
    printf("Moving to: %s\n", root->name);
    cwd = root;
    
  }
  else
  {
    // get dir_name, dir_name used by searchPath()
    sprintf(dir_name, "%s", pathname);
    
    printf("CD WITH PATH:%s\n", pathname);
    searchPath();
    printf("Moving to: %s\n", curr->name);
    cwd = curr;
  }
}
void rpwd(NODE *p)
{  
  if(strcmp(p->name, "root") == 0)
  {
    printf("root");
    return;
  }
  else
  {
    rpwd(p->parentPtr);
    printf("/%s", p->name);
    return;
  }
}
void pwd()
{  
  //printf("CWD: %s\n", cwd->name);
  rpwd(cwd);
  printf("\n");
}
void creat()
{ NODE *dir, *tempNode;
  char *token;
  char *lastToken;
  char *temp;
  int isAbsolute = 0;
  
  // Reset dirname and basename every loop
  sprintf(dir_name, "%s", "");
  sprintf(base_name, "%s", "");

  // Reset error condition
  errorOccurred = 0;
  
  if(pathname[0] == '/')
  {
    isAbsolute = 1;
  }

  token = strtok(pathname, "/");
  while(token)
  {
    lastToken = token;
    token = strtok(NULL, "/");

    if(token)
    {
      //append last to dir_name
      strcat(dir_name, lastToken);
      strcat(dir_name, "/");
    }
    else
    {
      //lastToken = basename
      sprintf(base_name, "%s", lastToken);
    }
  }
 
  // IF this is an Absolute pathname, append a / to the front
  if(isAbsolute)
  {
    temp = strdup(dir_name);
    strcpy(dir_name, "/");
    strcat(dir_name, temp);
  }
   
  // Search for dir_name node
  if(isAbsolute)
  {
    printf("Search mode is absolute\n");
    curr = root;
  }
  else
  {
    printf("Search mode is not absolute\n");
    curr = cwd;
  }
    
  searchPath();

  if(errorOccurred) // If there was an error during search, dont make a node.
  {
    return;
  }

  tempNode = curr->childPtr;
  while (tempNode != NULL)
  {
    if(strcmp(tempNode->name, base_name) == 0)
    {
      printf("ERROR: There is already a directory or file by this name\n");
      return;
    }
    tempNode = tempNode->siblingPtr;
  }  
  
  // ASSUME WE ARE IN THE RIGHT DIRECTORY //create new dir
  dir = malloc(sizeof(struct node));
  sprintf(dir->name, "%s", base_name);
  dir->nodeType = 'F';
  dir->parentPtr = curr;
  dir->siblingPtr = NULL;
  dir->childPtr = NULL;
  printf("dir->name: %s\ndir->nodeType:", dir->name); putchar(dir->nodeType); putchar('\n');
  printf("dir->parent: %s", dir->parentPtr->name); putchar('\n');
  if(curr->childPtr == NULL)
  {
    printf("There is no child, curr->childPtr = dir\n");
    curr->childPtr = dir;
  }
  else
  {    
    printf("There is a child, curr = curr->childPtr\n");
    curr = curr->childPtr;
    
    while(curr->siblingPtr != NULL)
    {
      printf("Curr has a sibling, curr = curr->siblingPtr\n");
      curr = curr->siblingPtr;
    }

    printf("Curr has no more siblings, curr->siblingPtr = dir\n");
    curr->siblingPtr = dir;
  }
  
}
void rm()
{
  NODE *tempNode, *lastTempNode;
  char *token;
  char *lastToken;
  char *temp;
  int isAbsolute = 0;
  
  // Reset dirname and basename every loop
  sprintf(dir_name, "%s", "");
  sprintf(base_name, "%s", "");

  // Reset error condition
  errorOccurred = 0;
  
  if(pathname[0] == '/')
  {
    isAbsolute = 1;
  }

  token = strtok(pathname, "/");
  while(token)
  {
    lastToken = token;
    token = strtok(NULL, "/");

    if(token)
    {
      //append last to dir_name
      strcat(dir_name, lastToken);
      strcat(dir_name, "/");
    }
    else
    {
      //lastToken = basename
      sprintf(base_name, "%s", lastToken);
    }
  }
 
  // IF this is an Absolute pathname, append a / to the front
  if(isAbsolute)
  {
    temp = strdup(dir_name);
    strcpy(dir_name, "/");
    strcat(dir_name, temp);
  }
   
  // Search for dir_name node
  if(isAbsolute)
  {
    printf("Search mode is absolute\n");
    curr = root;
  }
  else
  {
    printf("Search mode is not absolute\n");
    curr = cwd;
  }
    
  searchPath();

  if(errorOccurred) // If there was an error during search, dont delete a node.
  {
    return;
  }

  tempNode = curr->childPtr;
  if(tempNode->childPtr != NULL)
  {
    printf("ERROR: Directory to delete is not empty\n");
    return;   
  }

  printf("REMOVING: %s\n", pathname);

  //If here, delete node
  while(tempNode->siblingPtr != NULL)
  {
    if(strcmp(tempNode->name, pathname) == 0)
    {
      if(tempNode->siblingPtr != NULL)
      {
	printf("THERE IS A SIBLING. DONT ORPHAN THEM.\n");
	if(lastTempNode != NULL)
	{
	  printf("MIDDLE DELETION\n");
	  lastTempNode->siblingPtr = tempNode->siblingPtr;
	}
	else
	{
	  printf("FIRST DELETION\n");
	  curr->childPtr = tempNode->siblingPtr;
	}
	if(lastTempNode != NULL)
	{
	  printf("END DELETION\n");
	  // curr->childPtr = lastTempNode;
	}
	return;
      }
      else
      {
	printf("NO SIBLINGS. EXECUTE THEM.\n");
	curr->childPtr = NULL ;
	return;
      }
    }
    else
    {
      lastTempNode = tempNode;
      tempNode = tempNode->siblingPtr;
    }
  } 
}
void quit(int *waitingForQuit)
{
  *waitingForQuit = 0;
}
void help()
{
  printf("Help Menu\nAvailible Functions;\n\tmkdir\n\trmdir\n\tls\n\tcd\n\tpwd\n\tcreat\n\trm\n\tquit\n\thelp\n\t?\n\tmenu\n\treload\n\tsave\n");
}
void quest()
{
  printf("I don't know what the '?' is supposed to be!\n");
}
void menu()
{
  printf("Commands: mkdir, rmdir, ls, cd, pwd, creat, rm, quit, help, ?, menu, reload, save\n");
}
void reload()
{

}
void save()
{

}

int main(void)
{
  int waitingForQuit = 1;
  char *token;
  
  initialize();      /* initialize / node of the file system tree */

  while(waitingForQuit)
  {
    sprintf(command, "%s", "");
    sprintf(pathname, "%s", "");
    
    printf("input a command: ");
    //read a line containting  command [pathname]; // [ ] means optional
    fgets(line, 128, stdin);
    //Find the command string and call the corresponding function;
    token = strtok(line, "\n ");
    if(token)
    {
      sprintf(command, "%s", token);
      token = strtok(NULL, "\n ");
    }
    if(token)
    {
      sprintf(pathname, "%s", token);
    }  
    switch(findCmd(command))
    {
      case 0 : mkdir();                 break;
      case 1 : rmdir();                 break;
      case 2 : ls();                    break;
      case 3 : cd();                    break;
      case 4 : pwd();                   break;
      case 5 : creat();                 break;
      case 6 : rm();                    break;
      case 7 : quit(&waitingForQuit);   break;
      case 8 : help();                  break;
      case 9 : quest();                 break;
      case 10: menu();                  break;
      case 11: reload();                break;
      case 12: save();                  break;
      default: printf("ERROR: Command not found\n"); break;
    }
  }
}
