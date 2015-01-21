#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <sys/utsname.h>

#define BUFFSIZE 512
#define DEBUG false
#define DELIMITERS " \t"
#define SUCCESS 0
#define FAILURE 1
#define MAX_LENGTH 50
#define ARG_LENGTH 50
#define HISTORY_SIZE 100

static const char *history[20];
static  unsigned history_count = 0;
//struct for built-in shell functions
struct builtin
{
    const char* label;
    void (*op)(char**);  //function pointer to built-in's operation function
};

int countArgs(char* buffer)
{
    int word_count = 0;
    bool inword = false;
    const char* args = buffer;

    do switch(*args)
    {
        case '\0':
        case ' ': case '\t':
           if(inword) { inword = false; word_count++; }
           break;
        default: inword = true;
    } while(*args++);

    return word_count;
}

void parse(char* buffer, char** arguments)
{
    char* parsed = strtok(buffer, DELIMITERS);  //Break buffer into words
    char* ch;
    int i = 0;  //Index for char** arguments

    while (parsed != NULL)
    {
        ch = strrchr(parsed,'\n');  //Find last occurance of newline.
        if(ch)
            *ch = 0;  //Remove newline character
        arguments[i] = parsed;

        i++;
        parsed = strtok(NULL, DELIMITERS);  //Increment to next word
    }
}

bool check_builtins(struct builtin* bfunc, char* buffer, int bfunc_size)
{
    //returns true if buffer contains a built-in function, false otherwise.
    int arg_num = countArgs(buffer);
    char* arg[arg_num + 1];
    char input[BUFFSIZE] = {0};
    int i;

    strcpy(input, buffer);
    arg[arg_num] = NULL;
    parse(input, arg);  //Note: parse tokenizes buffer passed to it.

    for(i = 0; i < bfunc_size; i++)
    {
        if(strlen(arg[0])== strlen((bfunc[i]).label))
        {
            if (strcmp((bfunc[i]).label, arg[0]) == 0)
            {
                (bfunc[i]).op(arg);
                return true;
            }
        }
    }
    return false;
}

void close_shell(){exit(SUCCESS);}

void cd(char** arg){chdir(arg[1]);}

bool valid_file(char* filename)
{
    FILE* fptr = fopen(filename, "r");
    if (fptr != NULL)
    {
        fclose(fptr);
        return true;
    }
    return false;
}

bool valid_filename(char* filename)
{
    //a filename cannot contain characters used for redirection, piping,
    //or ';', ':', '/' or be NULL.
    char restricted_char[] = {'<', '>', '|', ';', ':', '/', 0};
    char* ptr_rchar =  restricted_char;

    while(*ptr_rchar)
    {
        if(strrchr(filename, *ptr_rchar)) return false;
        ptr_rchar++;
    }
    //check for NULL
    if(*ptr_rchar == *filename) return false;
    return true;
}

void check_redirection(char** arguments)
{
    char** arg = arguments;

    while(*arg)
    {
        //check for redirection of stdout with append feature.
        if(strcmp(*arg, ">>") == 0 )
        {
            *arg = NULL;    arg++;
            if(*arg && valid_filename(*arg))
                freopen(*arg, "a", stdout);
            else
            {
                fprintf(stderr, "Error: Bad file descriptor.\n");
                exit(FAILURE);    //kill child proccess.
            }
        }

        //check for redirection of stdout.
        if(strcmp(*arg, ">") == 0)
        {
            *arg = NULL;    arg++;
            if(*arg && valid_filename(*arg))
                freopen(*arg, "w", stdout);
            else
            {
                fprintf(stderr, "Error: Bad file descriptor.\n");
                exit(FAILURE);
            }
       }

        //check for redirection of stdin.
        if(strcmp(*arg, "<") == 0) {
            *arg = NULL;    arg++;
            if(*arg)
            {
                if(valid_file(*arg))
                   freopen(*arg, "r", stdin);
                else
                {
                    fprintf(stderr, "Error: %s does not exist.\n", *arg);
                    exit(FAILURE);
                }
            }
            else
            {
                fprintf(stderr, "Error: No file to redirect to.\n");
                exit(FAILURE);
            }
        }
        arg++;
    }
}

void run_pipe(char* arg1[], char* arg2[])
{
    int pfds[2];
    int k = pipe(pfds);
    // printf("this: %s", *arg);
    pid_t chp = fork();
    int stat;
    if(!chp)
    {
        //child code
        close(1);       /* close normal stdout */
        dup2(pfds[1], 1);   /* make stdout same as pfds[1] */
        close(pfds[0]); /* we don't need this */
        execvp(arg1[0], arg1);

        //only runs if exec fails
        fprintf(stderr, "%s: command not found\n",arg1[0]);
        return;
    }
    else
    {
        waitpid(chp, &stat, 0);
        close(0);       /* close normal stdin */
        dup2(pfds[0], 0);   /* make stdin same as pfds[0] */
        close(pfds[1]); /* we don't need this */
        execvp(arg2[0], arg2);

        //only runs if exec fails
        fprintf(stderr, "%s: Command not found.\n",arg2[0]);
        return;
    }
}

void check_piping(char** arguments)
{
    char** arg = arguments;   //point to first element of array
    char* targ1[10];
    char* targ2[10];
    int i = 0, f = 0;
    while(*arg)
    {
        if(strcmp(*arg, "|") == 0)
        {
            f = 1;
            targ1[i] = NULL;
            i = 0;
        }
        if(f==0)
            targ1[i++] = *arg;
        else if(strcmp(*arg, "|") != 0)
            targ2[i++] = *arg;
        arg++;
    }
    if(f)
        run_pipe(targ1, targ2);
    return;
}

void processDeleteCmd(char *secondCmd)
{
   // Remove the file
    if (remove(secondCmd)<0)
        printf("Unable to remove file\n");
    else
        printf("File successfully deleted.\n");
    return;
}

void cowsay(char** arguments)
{
    char** args = arguments;
    char* cow[] = {"     \\   ^__^                 ",                
               "      \\  (OO)\\__________    ",
               "         (__)\\          )\\/\\",
               "              | |HELLO| |  ",
               "              | |USERS| |   "};
    
    int numberOfChars = 0;
    int numArgs = 0;
    int i;
    while(*args)
    {   
        numberOfChars = strlen(*args) + numberOfChars;
        args++;
        numArgs++;
    }
        numberOfChars = numberOfChars-3;
    args = arguments;
    for(i = 0; i < numberOfChars + 4; i++)
        printf("_");
    printf("\n<  ");
    for(i = 1; i < numArgs; i++)
        printf("%s ", args[i]);
    printf("  >\n");
    for(i = 0; i < numberOfChars + 4; i++)
        printf("_");
    printf("\n");
    for(i = 0; i<5; i++)
        printf("%s\n", cow[i]);
    return;
}



int main(int argc, char** argv)
{
    struct builtin bfunc[] =
    {
        {.label = "exit", .op = &close_shell},
        {.label = "cd", .op = &cd}
    };
    int bfunc_size = (int) (sizeof(bfunc)/sizeof(bfunc[0]));

    //buffer is to hold user commands
    struct utsname ubuffer;
    unsigned index;
    char buffer[BUFFSIZE] = {0};
    char bufferTwo[BUFFSIZE] = {0};
    char cwd[BUFFSIZE] = {0};
    char username[BUFFSIZE] = {0};
    char* path[] = {"/bin/", "/usr/bin/", 0};

    uname(&ubuffer);    // now the ubuffer struct contains sys info

    while(1)
    {
        //printing the prompt
        if(getlogin_r(username, BUFFSIZE) == 0 && getcwd(cwd, BUFFSIZE) != NULL)
            printf("%s@%s:%s$ ", username, ubuffer.nodename, cwd);
        else
            printf("USER@COMPUTER: ");

        fgets(buffer, BUFFSIZE, stdin); // reading from command line

        // updating history
        if (history_count < 20)
            history[history_count++] = strdup(buffer);
        else
        {
            for(index=1; index<20; index++)
                history[index - 1] = history[index];
            history[19] = strdup(buffer);
        }

        if(strcmp(buffer, "history\n") == 0)
        {
            int n;
            for (n=1; n<20; n++)
            {
                if(history[n])
                    printf("History command  %d: %s", n, history[n]);
            }
            continue;
        }
        
        int numArgs = countArgs(buffer);
        char* args[numArgs+1];
        
        strncpy(bufferTwo, buffer,BUFFSIZE);
        parse(bufferTwo, args);
        args[numArgs] = NULL;

        // to delete 
        if (strcmp(args[0], "delete") == 0 && numArgs == 2)
            processDeleteCmd(args[1]);
        if (strcmp(args[0], "holycow") == 0)
            cowsay(args);

        if(!check_builtins(bfunc, buffer, bfunc_size))
        {
            int pid = fork();

            if(pid < 0)
                fprintf(stderr, "Unable to fork new process.\n");
            else if(pid > 0) //Parent code
                wait(NULL);
            else if(pid == 0)
            {
                //Child code
                int num_of_args = countArgs(buffer);
                //arguments to be passed to execv
                char* arguments[num_of_args+1];
                parse(buffer, arguments);

                if (strcmp(buffer,"starwars") == 0)
                {
                    char* argumentsNew[3];
                    argumentsNew[0] = "/usr/bin/telnet";
                    argumentsNew[1] = "towel.blinkenlights.nl";
                    argumentsNew[2] = NULL;

                    execv(argumentsNew[0], argumentsNew);
                }

                if(strcmp(arguments[0], "") == 0)
                    return(FAILURE);

                //Requirement of execv
                arguments[num_of_args] = NULL;
                check_piping(arguments);
                check_redirection(arguments);

                char prog[BUFFSIZE];
                char** path_p = path;

                while(*path_p)
                {
                    strcpy(prog, *path_p);

                    //Concancate the program name to path
                    strcat(prog, arguments[0]);
                    execv(prog, arguments);

                    path_p++;   //program not found. Try another path
                }

                if(strcmp(arguments[0], "history") == 0)
                {}
                else if ( strcmp(args[0], "delete") == 0 && numArgs == 2)
                {}
                else if ( strcmp(args[0], "holycow") == 0 )
                {}
                else
                {
                    //Following will only run if execv fails
                    fprintf(stderr, "%s: Command not found.\n",arguments[0]);
                }
               return FAILURE;
            }
        }
    }
    return SUCCESS;
}
