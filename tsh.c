#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <sys/utsname.h>
//#include <readline/history.h>

#define BUFFSIZE 512
#define DEBUG false
#define DELIMITERS " \t"
#define SUCCESS 0
#define FAILURE 1
#define UNIVERSAL_ZERO { 0 }
#define MAX_LENGTH 50
#define ARG_LENGTH 50
#define HISTORY_SIZE 100

static const char *history[11];
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

    do switch(*args) {
        case '\0':
        case ' ': case '\t':
           if(inword) { inword = false; word_count++; }
           break;
        default: inword = true;
    } while(*args++);

#if DEBUG
    printf("countArgs: Buffer contents\n");
    puts(buffer);
    printf("\ncountArgs: %d arguments\n", word_count);
#endif
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
        {
            *ch = 0;  //Remove newline character
        }
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
    char input[BUFFSIZE] = UNIVERSAL_ZERO;
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

void close_shell()
{
    exit(SUCCESS);
}

void cd(char** arg)
{
    chdir(arg[1]);   //will always be index 1 as 0 contains "cd".
}

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

bool valid_filename(char* filename) {
    //a filename cannot contain characters used for redirection, piping,
    //or ';', ':', '/' or be NULL.
    char restricted_char[] = {'<', '>', '|', ';', ':', '/', 0};
    char* ptr_rchar =  restricted_char;

    while(*ptr_rchar) {
        if(strrchr(filename, *ptr_rchar)) {
            return false;
        }
        ptr_rchar++;
    }
    //check for NULL
    if(*ptr_rchar == *filename) {
        return false;
    }
    return true;
}

void check_redirection(char** arguments) {
    char** arg = arguments;

    while(*arg) {
        //check for redirection of stdout with append feature.
        if(strcmp(*arg, ">>") == 0 ) {
            *arg = NULL;    arg++;
            if(*arg && valid_filename(*arg)) {
                freopen(*arg, "a", stdout);
            }
            else {
                fprintf(stderr, "Error: Bad file descriptor.\n");
                exit(FAILURE);    //kill child proccess.
            }
        }

        //check for redirection of stdout.
        if(strcmp(*arg, ">") == 0) {
            *arg = NULL;    arg++;
            if(*arg && valid_filename(*arg)) {
                freopen(*arg, "w", stdout);
            }
            else {
                fprintf(stderr, "Error: Bad file descriptor.\n");
                exit(FAILURE);
            }
       }

        //check for redirection of stdin.
        if(strcmp(*arg, "<") == 0) {
            *arg = NULL;    arg++;
            if(*arg) {
                if(valid_file(*arg)) {
                   freopen(*arg, "r", stdin);
                }
                else {
                    fprintf(stderr, "Error: %s does not exist.\n", *arg);
                    exit(FAILURE);
                }
            }
            else {
                fprintf(stderr, "Error: No file to redirect to.\n");
                exit(FAILURE);
            }
        }
        arg++;
    }
}

void run_pipe(char* arg[]) {
    int pfds[2];
    pipe(pfds);
    if(!fork()) {
        //child code
        close(1);       /* close normal stdout */
        dup(pfds[1]);   /* make stdout same as pfds[1] */
        close(pfds[0]); /* we don't need this */
        execlp(arg[0], arg[0], NULL);

        //only runs if exec fails
        fprintf(stderr, "%s: Command not found.\n",arg[0]);
        return;
    }
    else {
        close(0);       /* close normal stdin */
        dup(pfds[0]);   /* make stdin same as pfds[0] */
        close(pfds[1]); /* we don't need this */
        execlp(arg[2], arg[2], NULL);

        //only runs if exec fails
        fprintf(stderr, "%s: Command not found.\n",arg[2]);
        return;
    }
}

void check_piping(char** arguments) {
    char** arg = arguments;   //point to first element of array

    while(*arg) {
        if (strcmp(*arg, "|") == 0) {
            run_pipe(arguments);
            return;
        }
        arg++;
    }
}

void processDeleteCmd(char *secondCmd)
{
   // Remove the file
   if ( remove(secondCmd) < 0 )
   {
      printf("Unable to remove file\n");
      return;
   }
   else
   {
    printf("File successfully deleted.\n");
    return;
   }
}

void cowsay(char** arguments)
{
    char** args = arguments;
    char* cow[] = {"     \\   ^__^                 ",                
               "      \\  (OO)\\__________    ",
               "         (__)\\          )\\/\\",
               "              | |SHELL| |  ",
               "              | | ers | |   "};
    
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
    {
        printf("_");
    }
    printf("\n<  ");
    for(i = 1; i < numArgs; i++)
    {
        printf("%s ", args[i]);
    }
    printf("  >\n");
    for(i = 0; i < numberOfChars + 4; i++)
    {
        printf("_");
    }
    printf("\n");
    for(i = 0; i<5; i++)
    {
        printf("%s\n", cow[i]);
    }
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
    char buffer[BUFFSIZE] = UNIVERSAL_ZERO;  //zero every elerment of the buffer
    char bufferTwo[BUFFSIZE] = UNIVERSAL_ZERO;
    char cwd[BUFFSIZE] = UNIVERSAL_ZERO;
    char username[BUFFSIZE] = UNIVERSAL_ZERO;
    char* path[] = {"/bin/", "/usr/bin/", 0};

    uname(&ubuffer);

    while(1)
    {
        //print the prompt
        if(getlogin_r(username, BUFFSIZE) == 0 && getcwd(cwd, BUFFSIZE) != NULL)
            printf("%s@%s:~%s$ ", username, ubuffer.nodename, cwd);
        else
            printf("myShell&gt: ");

        fgets(buffer, BUFFSIZE, stdin);
        if (history_count < 11)
        {
            history[history_count++] = strdup(buffer);
        }
        else
        {
            //free( history[0] );
            for (index = 1; index < 11; index++) {
                history[index - 1] = history[index];
                history[11 - 1] = strdup(buffer);
            }
        }

        if(strcmp(buffer, "history\n") == 0) {
            int n;
            for (n = 1; n < 10; n++) {
                printf("History command  %d: %s\n", n, history[n]);
            }
        }
        
        int numArgs = countArgs(buffer);
    char* args[numArgs+1];
    strncpy(bufferTwo, buffer,BUFFSIZE);
        parse(bufferTwo, args);
    args[numArgs] = NULL;
    if ( strcmp(args[0], "delete") == 0 && numArgs == 2)
        {
                processDeleteCmd(args[1]);
        }
    if ( strcmp(args[0], "holycow") == 0)
    {
        cowsay(args);
    }



        if(!check_builtins(bfunc, buffer, bfunc_size))
        {
            int pid = fork();

            if(pid < 0)
                fprintf(stderr, "Unable to fork new process.\n");
            if(pid > 0) //Parent code
                wait(NULL);
            if(pid == 0)
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
                {
                    return(FAILURE);
                }

                //Requirement of execv
                arguments[num_of_args] = NULL;
                check_piping(arguments);
                check_redirection(arguments);

                char prog[BUFFSIZE];
                char** path_p = path;

                while(*path_p) {
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
