#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<unistd.h>
#include<signal.h>
#include<limits.h>

#define MAX_ARGS 10
#define MAX_LEN  100

void printprompt();
char *readl(void);
void handle_signal(int signo){fflush(stdout);}
int main(int argc, char *argv[])
{
    int n = 0;
    int status;
    char *cp;
    const char *delim = " \t"; /* delimiters of commands */

    while(1)
    {
        /* show a prompt */
        printprompt();
        
        signal(SIGINT, SIG_IGN);
        signal(SIGINT, handle_signal);

        /* read a line */
        cp = readl();
        if(cp[0]=='\0')continue;
        /* split the input to commands with blank and Tab */
        for (argc = 0; argc < MAX_ARGS; argc++)
        {
            argv[argc] = NULL;
            if((argv[argc] = strtok(cp, delim)) == NULL)
                break;
            cp = NULL;
        }
        argc--;
        
        /* exit this shell when "exit" is inputed */
        if(strcmp(argv[0], "exit") == 0)
            return 0;
        else if(strcmp(argv[0], "cd") == 0)
        {
            int st = chdir(argv[1]);
            if(st!=0)perror("Error");
            continue;
        }
        
        pid_t pid = fork();
        if(pid == 0)
        {
            int st = execvp(argv[0], argv);
            if(st==-1)
            {
                printf("%s: command not found\n", argv[0]);
                return 0;
            }
        }
        else
        {
            /* background job */
            waitpid(pid, &status, 0);
        }
    }
    return 0;
}
void printprompt()
{
    char buff[PATH_MAX+1];
    char *usr = getlogin(), *dir = getcwd(buff, PATH_MAX+1);
    printf("%s@computer:%s$ ", usr, dir);
}
char *readl(void)
{
    char * line = malloc(100), * linep = line;
    size_t lenmax = 100, len = lenmax;
    int c;
    if(line == NULL)return NULL;
    for(;;)
    {
        c = fgetc(stdin);
        if(c == EOF)break;
        if(--len == 0)
        {
            len = lenmax;
            char * linen = realloc(linep, lenmax *= 2);
            if(linen == NULL)
            {
                free(linep);
                return NULL;
            }
            line = linen + (line - linep);
            linep = linen;
        }
        if((*line++ = c) == '\n')break;
    }
    line--;*line = '\0';
    return linep;
}
