#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>

#define MAX_PIPES 128
#define PIPELINE_FLAG (1 << 0) // -> 0001
#define REDIRECTION_FLAG (1 << 1) // -> 0010

unsigned int flags = ~(PIPELINE_FLAG | REDIRECTION_FLAG);
// 0000 if all flags are deactivated, 0011 if all flags are activated

int pipeIndexes[MAX_PIPES];
int pipeIndex;
int numPipes;


void parser(char *input, char *argv[]){ //parse the user's input
        int i, j = 0, k = 0;
        bool word = false;
        for (i = 0; input[i] != '\0'; i++){
            if (input[i] == ' '){
                input[i] = '\0';
                word = false;
                continue;
            }

            if (input[i] == '|'){
                argv[j] = "|";
                word = false;
                flags |= PIPELINE_FLAG;
                pipeIndex = j;
                pipeIndexes[k] = pipeIndex;
                numPipes++;
                j++;
                k++;
                continue;

            }

            if (input[i] == '>' && input[i+1] == '>'){
                argv[j] = ">>";
                j++;
                word = false;
                i++; 
                continue;
            }

            if (input[i] == '>'){
                argv[j] = ">";
                j++;
                word = false;
                continue;
            }

            if (input[i] == '<'){
                argv[j] = "<";
                j++;
                word = false;
                continue;
            }

            if (input[i] != ' ' && word == false){
                    argv[j] = &input[i];
                    j++;

            if (input[i] != ' ')
                word = true;
            }
        }
    argv[j] = NULL;
}

void printPrompt(void){
	
    char buffer[256];
    char *user = getenv("USER"); //get the user's username
    snprintf(buffer, sizeof(buffer), "/home/%s", user);
    char *cwd = getcwd(NULL, 0);
    char host[HOST_NAME_MAX + 1];
    gethostname(host, sizeof(host));
    strcmp(cwd, buffer) == 0 ? printf("\x1B[1;36m%s@%s\x1B[0m:~$ ", user, host)
    : printf("\x1B[1;36m%s@%s\x1B[0m:%s$ ", user, host, cwd);
    free(cwd);
}


int main(int argc, char **argv)
{
    char input[255];
  
    puts("You are now using \x1B[1;36mradish\x1B[0m made by \x1B[1;36mHFMaker\x1B[0m");
    

    while (1) //Main loop
    {
        printPrompt();
        fflush(stdout);
        if (!fgets(input, MAX_INPUT, stdin)) break;
        
    input[strcspn(input, "\n")] = 0;

    if (strlen(input) == 0) continue;

    for (int i = 0; pipeIndexes[i] != '\0'; i++) pipeIndexes[i] = 0;
    numPipes = 0;
    pipeIndex = 0;


    char *args[MAX_INPUT]; //User's input is stored here

    parser(input, args); //

    //Some built-in commands and some new commands are down below
     
    if (strcmp(args[0], "cd") == 0){ 
         if (!args[1]){
            chdir(getenv("HOME")); //get the user's home directory
         } else{
             if (chdir(args[1]) != 0){
                perror("cd");
            }
         }
        continue;
        
    }

    
    if (strcmp(args[0], "exit") == 0){
        exit(0); //exti the program 
    }

    if (strcmp(args[0], "openshell") == 0){ //
        pid_t pid = fork();

        if (pid == 0)
        {
            char *term = getenv("TERMINAL"); //get the user's default terminal

            if (term){
                execlp(term, term, "-e", "./hf-shell", NULL);
                execlp(term, term, "--", "./hf-shell", NULL);
            }

            execlp("x-terminal-emulator", "x-terminal-emulator", "-e", "./hf-shell", NULL);
            execlp("kitty", "kitty", "-e", "./hf-shell", NULL);
            execlp("konsole", "konsole", "-e", "./shell", NULL);
            execlp("gnome-terminal", "gnome-terminal", "--", "./hf-shell", NULL);
            execlp("xfce4-terminal", "xfce4-terminal", "-e", "./hf-shell", NULL);
            execlp("alacritty", "alacritty", "-e", "./hf-shell", NULL);
            execlp("mate-terminal", "mate-terminal", "-e", "./hf-shell", NULL);
            execlp("xterm", "xterm", "-e", "./hf-shell", NULL);
            perror("Compatible terminal not found");
            exit(1);
            execvp(args[0], args);
            perror("execvp");
            exit(1);
        }
    

        wait(NULL);
        continue;
        
    }
 
    if (flags & PIPELINE_FLAG){ //Execute the user's input if there's a pipeline
                
            int procs = numPipes + 1; //Number of process
            int fd[2 * numPipes];
            int fdIndex = 0;
            int start[procs];
            start[0] = 0;
            for (int i = 0; i < numPipes; i++){
                if (pipe(&fd[2 * i]) == -1){
                    printf("Error while doing the pipe");
                    return 0;
                }   
            }

            for (int i = 0; pipeIndexes[i] != '\0'; i++) args[pipeIndexes[i]] = NULL;

            pid_t pids[procs];

            for (int i = 0; i < procs; i++){

                pids[i] = fork();


                if (pids[i] == 0) {

                    if (i == 0) dup2(fd[1], STDOUT_FILENO);
                    else if (i == procs - 1) dup2(fd[2 * (numPipes - 1)], STDIN_FILENO);
                    else {dup2(fd[2 * (i - 1)], STDIN_FILENO); dup2(fd[2 * i + 1], STDOUT_FILENO);}
                    for (int i = 0; i < 2 * numPipes; i++) close(fd[i]);
                    for (int i = 1; i < procs; i++) start[i] = pipeIndexes[i - 1] + 1;

                    execvp(args[start[i]], &args[start[i]]);
                    perror("execvp");
                    exit(1);
                    

                }

            }
            
            for (int i = 0; i < 2 * numPipes; i++) close(fd[i]);
            for (int i = 0; i < procs; i++) wait(NULL);
            flags &= ~PIPELINE_FLAG;
            continue;


    }

    int redirection_type = 0; //Check if the user's input has any redirection
    int j;
    int redirection_index;
    for (j = 0; args[j] != NULL; j++){
         if (strcmp(args[j], ">") == 0){
            redirection_type = 1;
            redirection_index = j;
            args[j] = NULL;
         }

         else if (strcmp(args[j], ">>") == 0){
            redirection_type = 2;
            redirection_index = j;
            args[j] = NULL;
         }

         else if (strcmp(args[j], "<") == 0){
            redirection_type = 3;
            redirection_index = j;
            args[j] = NULL;
         }
    }

    if (redirection_type != 0){//Execute the user's input if there's a redirection
        pid_t pid = fork();
        if (pid == 0){
            int fd_archivo;
            if (redirection_type == 1){
                fd_archivo = open(args[redirection_index + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            }

            else if (redirection_type == 2){
                fd_archivo = open(args[redirection_index + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            }
            
            else if (redirection_type == 3){
                fd_archivo = open(args[redirection_index + 1], O_RDONLY, 0644);
            }
            
            
            if (fd_archivo == -1){
                perror("open");
                exit(1);
            }
            if (redirection_type == 1 || redirection_type == 2){
                dup2(fd_archivo, STDOUT_FILENO);
            }
            else if (redirection_type == 3){
                dup2(fd_archivo, STDIN_FILENO);
            }
            if (close(fd_archivo) == -1){   
                perror("close");
                exit(1);
            }
            execvp(args[0], args);
        }
        
        wait(NULL);
        continue;
    }

    
    pid_t pid = fork();// Execute the user's input if there's no pipeline or redirection

    if (pid == 0){
        execvp(args[0], args);
        printf("radish: %s: command not found\n", args[0]);
        exit(1);
    }
    else wait(NULL);
    
    }
return 0; 
}



