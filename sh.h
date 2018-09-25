
#include "get_path.h"

int pid;
int sh( int argc, char **argv, char **envp);
char* cd(char **args, char* homedir, char* prevDir, char* pwd);
char *which(char *command, struct pathelement *pathlist);
char *where(char *command, struct pathelement *pathlist);
void list ( char *dir );
void printenv(char **envp);

#define PROMPTMAX 32
#define MAXARGS 10
