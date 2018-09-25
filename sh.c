#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "sh.h"

#define BUFFERSIZE 256

int sh( int argc, char **argv, char **envp )
{
  char *prompt = calloc(PROMPTMAX, sizeof(char));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *commandpath, *p, *pwd, *owd, *prevDir;
  char **args = calloc(MAXARGS, sizeof(char*));
  int uid, i, status, argsct, go = 1;
  struct passwd *password_entry;
  char *homedir;
  struct pathelement *pathlist;
  char* upDir = "..";
  char* thisDir = ".";

  uid = getuid();
  password_entry = getpwuid(uid);               /* get passwd info */
  homedir = password_entry->pw_dir;		/* Home directory to start
						  out with*/

  if ( (pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
  {
    perror("getcwd");
    exit(2);
  }
  owd = calloc(strlen(pwd) + 1, sizeof(char));
  memcpy(owd, pwd, strlen(pwd));
  prompt[0] = ' '; prompt[1] = '\0';
  prevDir = (char*) malloc(strlen(pwd));
  strcpy(prevDir, pwd);

  /* Put PATH into a linked list */
  pathlist = get_path();

  while ( go )
  {
    /* print your prompt */

    /* get command line and process */
    fgets(commandline,  MAX_CANON, stdin);
    if (0 == strcmp(commandline, "\n")) {
      continue;
    }
    commandline[strcspn(commandline, "\n")] = 0;// strip newline if it exists
    const char space[2] = " ";
    command = strtok(commandline, space);

    char* newArg = command;
    int argCounter = 0;
    while (newArg != NULL) {
      newArg = strtok(NULL, space);
      args[argCounter] = newArg;
      argCounter++;
    }

    /* check for each built in command and implement */
    if (
	0 == strcmp(command, "exit") ||
	0 == strcmp(command, "which") ||
	0 == strcmp(command, "where") ||
	0 == strcmp(command, "cd") ||
	0 == strcmp(command, "pwd") ||
	0 == strcmp(command, "list") ||
	0 == strcmp(command, "pid") ||
	0 == strcmp(command, "kill") ||
	0 == strcmp(command, "prompt") ||
	0 == strcmp(command, "printenv") ||
	0 == strcmp(command, "alias") ||
	0 == strcmp(command, "history") ||
	0 == strcmp(command, "setenv")) {
      printf("Executing built-in ");
    }

    if (0 == strcmp(command, "exit")) {
      printf("exit\n");
      return 0;
    }
    else if (0 == strcmp(command, "which")) {
      printf("which\n");
      if (NULL == args[0]) {
        continue;
      }
      char* result = which(args[0], pathlist);
      if (NULL != result) {
        printf("%s\n", result);
      }
      free(result);
    }
    else if (0 == strcmp(command, "where")) {
      printf("where\n");
      if (NULL == args[0]) {
        continue;
      }
      where(args[0], pathlist);
    }
    else if (0 == strcmp(command, "cd")) {
      printf("cd\n");
      char* targetDir = calloc(sizeof(char), BUFFERSIZE);
      if (args[0] == NULL) {
        strcpy(targetDir, homedir);
      }
      else if ('~' == args[0][0] ) {
        strcat(targetDir, homedir);
        strcat(targetDir, args[0]+1);
      }
      else if ('/' == args[0][0]) {
        strcpy(targetDir, args[0]);
      }
      else if (0 == strcmp(thisDir, args[0])) {
        continue;
      }
      else if (0 == strcmp(upDir, args[0])) {
        char* lastSlash = strrchr(pwd, '/');
        strncpy(targetDir, pwd, strlen(pwd) - strlen(lastSlash));
        targetDir[strlen(pwd) - strlen(lastSlash)] = '\0';
      }
      else if ('-' == args[0][0]) {
        strcpy(targetDir, prevDir);
      }
      else {
        strcat(targetDir, pwd);
        strcat(targetDir, "/");
        strcat(targetDir, args[0]);
      }
      DIR* folder = opendir(targetDir);

      closedir(folder);
      if (NULL != folder) {
        prevDir = (char*) malloc(sizeof(pwd));
        strcpy(prevDir, pwd);
        strcpy(pwd, targetDir);
      }
      else if (ENOENT == errno) {
        printf("No such directory: %s\n", targetDir);
      }
      else {
        printf("Something went wrong\n");
      }
      free(targetDir);
    }
    else if (0 == strcmp(command, "pwd")) {
      printf("pwd\n");
      printf("%s\n", pwd);
    }
    else if (0 == strcmp(command, "list")) {
    }
    else if (0 == strcmp(command, "pid")) {
    }
    else if (0 == strcmp(command, "kill")) {
    }
    else if (0 == strcmp(command, "prompt")) {
    }
    else if (0 == strcmp(command, "printenv")) {
    }
    else if (0 == strcmp(command, "alias")) {
    }
    else if (0 == strcmp(command, "history")) {
    }
    else if (0 == strcmp(command, "setenv")) {
    }

     /*  else  program to exec */
    {
       /* find it */
       /* do fork(), execve() and waitpid() */

      if(1) {
      }
      else
        fprintf(stderr, "%s: Command not found.\n", args[0]);
    }
  }
  return 0;
} /* sh() */

char *which(char *command, struct pathelement *pathlist )
{
   /* loop through pathlist until finding command and return it.  Return
   NULL when not found. */
  while (NULL != pathlist->next) {
    DIR* folder = opendir(pathlist->element);
    if (NULL != folder) {
      struct dirent* dirEntry;
      while (NULL != (dirEntry = readdir(folder))) {
	if (0 == strcmp(command, dirEntry->d_name)) {
	  char* result = (char*) malloc(sizeof(char) * (strlen(pathlist->element) + strlen(command)));
	  strcat(result, pathlist->element);
	  strcat(result, "/");
	  strcat(result, command);
	  return result;
	}
      }
    }
    pathlist = pathlist->next;
  }
  return NULL;
} /* which() */

char *where(char *command, struct pathelement *pathlist )
{
  /* similarly loop through finding all locations of command */
  while (NULL != pathlist->next) {
    DIR* folder = opendir(pathlist->element);
    if (NULL != folder) {
      struct dirent* dirEntry;
      while (NULL != (dirEntry = readdir(folder))) {
	if (0 == strcmp(command, dirEntry->d_name)) {
	  char* result = (char*) malloc(sizeof(char) * (strlen(pathlist->element) + strlen(command)));
	  strcat(result, pathlist->element);
	  strcat(result, "/");
	  strcat(result, command);
	  printf("%s\n", result);
	  free(result);
	}
      }
    }
    pathlist = pathlist->next;
  }
  return NULL;
} /* where() */

void list ( char *dir )
{
  /* see man page for opendir() and readdir() and print out filenames for
  the directory passed */
} /* list() */

