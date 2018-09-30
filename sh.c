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
#include <glob.h>
#include "sh.h"

#define BUFFERSIZE 256

int sh( int argc, char **argv, char **envp )
{
  char *prompt = calloc(PROMPTMAX, sizeof(char));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *commandlinecopy = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *commandpath, *p, *pwd, *owd, *prevDir;
  char **args = calloc(MAXARGS, sizeof(char*));
  char **execargs = calloc(MAXARGS + 1, sizeof(char*));
  int uid, i, status, argsct, go = 1;
  struct passwd *password_entry;
  char *homedir;
  struct pathelement *pathlist;
  char* rootDir = "/";
  char* upDir = "..";
  char* thisDir = ".";
  char* space = " ";
  char* path = "PATH";
  char* home = "HOME";
  char* wildcard = "*";
  char* singlewildcard = "?";
  char* dash = "-";
  struct historyelement *lastcommand = NULL;
  struct historyelement *newcommand = NULL;
  struct aliaselement* aliasList = NULL;

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
  prompt[0] = '\0';
  prevDir = (char*) malloc(BUFFERSIZE);
  strcpy(prevDir, pwd);

  /* Put PATH into a linked list */
  pathlist = get_path();

  while ( go )
  {
    /* print your prompt */
    printf("%s [%s]>", prompt, pwd);

    /* get command line and process */
    fgets(commandline,  MAX_CANON, stdin);
    if (0 == strcmp(commandline, "\n")) {
      continue;
    }
    commandline[strcspn(commandline, "\n")] = 0;// strip newline if it exists
    newcommand = historyCommand(commandline);
    newcommand->prev = lastcommand;
    if (NULL != lastcommand) {
      lastcommand->next = newcommand;
    }
    lastcommand = newcommand;
    const char space[2] = " ";
    strcpy(commandlinecopy, commandline);
    command = strtok(commandline, space);

    char* newArg = command;
    int argCounter = 0;
    while (newArg != NULL) {
      newArg = strtok(NULL, space);
      args[argCounter] = newArg;
      argCounter++;
    }
    argsct = argCounter;

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
      break;
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
      cd(args[0], homedir, prevDir, pwd);
    }
    else if (0 == strcmp(command, "pwd")) {
      printf("pwd\n");
      getcwd(pwd, BUFFERSIZE);
      printf("%s\n", pwd);
    }
    else if (0 == strcmp(command, "list")) {
      printf("list\n");
      if (NULL == args[0]) {
        // print current dir files
        list(pwd);
      }
      else {
        // print files in args[] directories
        char* prevCopy = calloc(sizeof(char), strlen(prevDir) + 1);
        strcpy(prevCopy, prevDir);
        for (int i = 0; i < argsct - 1; i++) {
          cd(args[i], homedir, prevCopy, pwd);
          printf("\n%s:\n\n", pwd);
          list(pwd);
          cd("-", homedir, prevCopy, pwd);
        }
        free(prevCopy);
      }
    }
    else if (0 == strcmp(command, "pid")) {
      printf("pid\n");
      int pid = getpid();
      printf("%d\n", pid);
    }
    else if (0 == strcmp(command, "kill")) {
      printf("kill\n");
      if (2 == argsct) {
        int pid = atoi(args[0]);
        kill(pid, 15);
      }
      else if (3 == argsct) {
        int pid = atoi(args[0]);
        args[1]++;
        int sig = atoi(args[1]);
        kill(pid, sig);
      }
    }
    else if (0 == strcmp(command, "prompt")) {
      printf("prompt\n");
      if (NULL != args[0]) {
        strncpy(prompt, args[0], PROMPTMAX);
      }
      else {
        printf("prompt: ");
        fgets(prompt, PROMPTMAX, stdin);
        prompt[strlen(prompt) - 1] = '\0';
      }
    }
    else if (0 == strcmp(command, "printenv")) {
      printf("printenv\n");
      if (NULL != args[1]) {
        printf("printenv: Too many arguments.\n");
      }
      else if (NULL != args[0]) {
        char* result = getenv(args[0]);
        printf("%s\n", result);
      }
      else {
        printenv(envp);
      }
    }
    else if (0 == strcmp(command, "alias")) {
      printf("alias\n");
      if (NULL != args[1] && NULL != args[0]) {
        struct aliaselement* newAlias = addAlias(args[0], args+1, argsct-2);
        if (aliasList) {
          newAlias->next = aliasList;
          aliasList = newAlias;
        }
        else {
          aliasList = newAlias;
        }
      }
      else if (NULL == args[0]) {
        struct aliaselement* alias = aliasList;
        while(NULL != alias) {
          printf("%s\t%s ", alias->command, alias->expansion[0]);
          for (int i = 1; NULL != alias->expansion[i]; i++) {
            printf("%s ", alias->expansion[i]);
          }
          printf("\n");
          alias = alias->next;
        }
      }
      else {
        printf("alias: requires 0 arguments, or 2\n");
      }
    }
    else if (0 == strcmp(command, "history")) {
      printf("history\n");
      int count = 10;
      if (NULL != args[0]) {
        count = atoi(args[0]);
      }
      struct historyelement *he = lastcommand;
      while(NULL != he->prev && 0 < count) {
        he = he->prev;
        count--;
      }
      count = 0;
      while(NULL != he->next) {
        count++;
        printf("%d: %s\n", count, he->command);
        he = he->next;
      }
    }
    else if (0 == strcmp(command, "setenv")) {
      printf("setenv\n");
      if (NULL != args[1]) {
        setenv(args[0], args[1], 1);
      }
      else if (NULL != args[0]) {
        char* empty = "";
        int result = setenv(args[0], empty, 0);
      }
      else {
        printenv(envp);
      }
      if (NULL != args[0] && 0 == strcmp(args[0], path)) {
        struct pathelement *newPathlist = get_path();
        struct pathelement *next;
        while(NULL != pathlist->next) {
          next = pathlist->next;
          free(pathlist);
          pathlist = next;
        }
        pathlist = newPathlist;
      }
      if (NULL != args[0] && 0 == strcmp(args[0], home)) {
        homedir = getenv(home);
      }
    }



    else {
      /*  else  program to exec */
      char* com = malloc(BUFFERSIZE);

      // check if it matches an alias in the alias table
      struct aliaselement* alias = aliasList;
      while(alias) {
        if (0 == strcmp(command, alias->command)) {
          for (int i = MAXARGS - 1; i > 0; i--) {
            args[i] = args[i-alias->parts];
          }
          for (int i = 0; i < alias->parts; i++) {
            args[i] = alias->expansion[i + 1];
          }
          strncpy(command, alias->expansion[0], strlen(alias->expansion[0]));
          break;
        }
        alias = alias->next;
      }

      /* find it */
      if ((0 == strncmp(thisDir, command, 1)) || (0 == strncmp(rootDir, command, 1)) || (0 == strncmp(upDir, command, 2))) {
        snprintf(com, strlen(command) + 1, "%s", command);
      }
      else {
        com = which(command, pathlist);
      }
      /* do fork(), execve() and waitpid() */

      if(NULL != com && (0 == access(com, X_OK))) {
        printf("Executing %s\n", com);
        pid_t parent = getpid();
        pid_t pid = fork();
        if (pid > 0) {
          int status;
          int outputStatus = 0;
          waitpid(pid, &status, 0);
          if (0 != (outputStatus = WEXITSTATUS(status))) {
            printf("%d\n", outputStatus);
          }
        }
        else {
          char** const envp = {NULL};
          execargs[0] = com;
          if (0 == strstr(commandlinecopy, wildcard) &&
              0 == strstr(commandlinecopy, singlewildcard)) {
            for (int i = 0; i < argsct; i++) {
              execargs[i+1] = args[i];
            }
            if (-1 == execve(com, execargs, envp)) {
              perror("Failed");
            }
          }
          else {
            glob_t globbuf;
            int gl_offs_count = 1;
            for (int i = 0; i < argsct - 1; i++) {
              if (0 == strncmp(args[i], dash, 1)) {
                gl_offs_count++;
              }
            }
            globbuf.gl_offs = gl_offs_count;
            glob(args[argsct-gl_offs_count-1], GLOB_DOOFFS, NULL, &globbuf);
            for (int i = 1; i < gl_offs_count; i++) {
              glob(args[argsct-gl_offs_count], GLOB_DOOFFS | GLOB_APPEND, NULL, &globbuf);
            }
            globbuf.gl_pathv[0] = (char*) malloc(strlen(command));
            strcpy(globbuf.gl_pathv[0], command);
            int idx = 1;
            for (int i = 0; i < MAXARGS; i++) {
              if (NULL != args[i] &&
                  0 == strstr(args[i], wildcard) &&
                  0 == strstr(args[i], singlewildcard)) {
                globbuf.gl_pathv[idx] = (char*) malloc(strlen(args[i]));
                globbuf.gl_pathv[idx] = args[i];
                idx++;
              }
            }
            if (-1 == execvp(com, &globbuf.gl_pathv[0])) {
              perror("Failed");
            }
            globfree(&globbuf);
          }
        }

        free(com);
      }
      else
        fprintf(stderr, "%s: Command not found.\n", command);
    }
  }
  do {
    struct historyelement* prev = lastcommand->prev;
    free(lastcommand);
    lastcommand = prev;
  } while(lastcommand->prev);
  free(prompt);
  free(commandline);
  free(commandlinecopy);
  free(owd);
  free(pwd);
  free(prevDir);
  free(args);
  return 0;
} /* sh() */

void cd(char *args, char* homedir, char* prevDir, char* pwd) {
  int result = -1;
  char* pd;
  pd = calloc(sizeof(char), BUFFERSIZE);
  getcwd(pd, BUFFERSIZE);
  if (NULL == args) {
    result = chdir(homedir);
  }
  else if ('~' == args[0]) {
    args++;
    char* path = calloc(sizeof(char), strlen(homedir) + strlen(args) + 1);
    strcat(path, homedir);
    strcat(path, args);
    result = chdir(path);
    free(path);
  }
  else if ('-' == args[0]) {
    if (NULL == prevDir || '\0' == prevDir[0]) {
      return ;
    }
    result = chdir(prevDir);
  }
  else {
    result = chdir(args);
  }
  if (0 == result) {
    strcpy(prevDir, pd);
    prevDir[strlen(pd)] = '\0';
    char* newPwd = calloc(sizeof(char), BUFFERSIZE);
    getcwd(newPwd, BUFFERSIZE);
    strcpy(pwd, newPwd);
    pwd[strlen(newPwd)] = '\0';
    free(newPwd);
  }
  else {
    perror("Something went wrong");
  }
  free(pd);
}

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
          char* result = (char*) malloc(sizeof(char) * BUFFERSIZE);
          snprintf(result, BUFFERSIZE, "%s/%s", pathlist->element, command);
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
  DIR* folder = opendir(dir);
  struct dirent* dirEntry;
  while (NULL != (dirEntry = readdir(folder))) {
    printf("%s\n", dirEntry->d_name);
  }
  closedir(folder);
} /* list() */

void printenv(char** envp) {
  for (int i = 0; NULL != envp[i]; i++) {
    printf("%s\n", envp[i]);
  }
}
