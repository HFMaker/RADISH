/*
RADISH - Real And Definitive Implementation of a SHell
Copyright (C) 2026  HFMaker

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your opinion) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/


/* ~~~ TODO list ~~~ */

/*
 * Finish the pipes + redirections implementation (right now it only works for the last argument)
 * Implement signal handling (for now Ctrl+C and Ctrl+Z will be enough)
 * 
 *
 *
 *
 * */



/* ~~~ #include DECLARATIONS ~~~ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>

/* ~~~ MACROS AND FLAGS DECLARATIONS ~~~ */

#define MAX_PIPES 128
#define MAX_REDIRS 128
#define PIPELINE_FLAG (1 << 0) // -> 0001
#define REDIRECTION_FLAG (1 << 1) // -> 0010

/* ~~~ GLOBAL VARIABLES DECLARATIONS ~~~ */

unsigned int flags = ~(PIPELINE_FLAG | REDIRECTION_FLAG);
// 0000 if all flags are deactivated, 0011 if all flags are activated

unsigned int redirFlags; 

//arrays where all the pipes and redirections on the user's input are stored
int pipeIndexes[MAX_PIPES]; 
int redirIndexes[MAX_REDIRS];

//Where a pipe or redirection is located on the user's input
int pipeIndex;
int redirIndex;

//Number of pipes and redirections on the user's input
int numPipes;
int numRedirs;

//Process corresponding to a redirection

int redirProc;

static struct termios ogTermios;


/* ~~~ GLOBAL FUNCTIONS DECLARATIONS ~~~ */



void rawModeON(){
    
    tcgetattr(STDIN_FILENO, &ogTermios);
    struct termios term = ogTermios;
    term.c_lflag &= ~(ICANON | ECHO);
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void rawModeOFF(){
    tcsetattr(STDIN_FILENO, TCSANOW, &ogTermios);
}




/*ARGUMENTS EXPLANATION:
 *
 * *input -> unmodified user input
 * *argv[] -> array where the user input is going to be modified
 */

void parser(char *input, char *argv[]){ //parse the user's input
        int i, j = 0, k = 0, l = 0;
        bool word = false;
        for (i = 0; input[i] != '\0'; i++){ 
            if (input[i] == ' '){
                input[i] = '\0';
                word = false;
                continue;
            }

            else if (input[i] == '|'){ //check if there's a pipe character in the input
                argv[j] = "|";
                word = false;
                flags |= PIPELINE_FLAG; //activate the pipeline flag...
                pipeIndex = j;//...save the pipe position to a pipe index
                pipeIndexes[k] = pipeIndex;//and add that pipe to an array with the pipe indexes
                numPipes++;
                j++;
                k++;
                continue;

            }

            else if (input[i] == '>' && input[i+1] == '>'){ //check if there's a >> in the input
                argv[j] = ">>";
                word = false;
                redirIndex = j;
                redirIndexes[l] = redirIndex;
                numRedirs++;
		        j++;
                l++;
                continue;
            }

            else if (input[i] == '>'){
                argv[j] = ">";
                word = false;
                redirIndex = j;
                redirIndexes[l] = redirIndex;
                numRedirs++;
                j++;
                l++;
                continue;
            }

            else if (input[i] == '<'){
                argv[j] = "<";
                word = false;
                redirIndex = j;
                redirIndexes[l] = redirIndex;
                numRedirs++;
                j++;
                l++;
                continue;
            }

            if (input[i] != ' ' && word == false){
                    argv[j] = &input[i];
		            word = true;
                    j++;
            }
        }
    argv[j] = NULL;
}

void printPrompt(void){ //print the user's promt
	
    char currentDir[256];
    char *user = getenv("USER"); 
    char *cwd = getcwd(NULL, 0);
    char host[HOST_NAME_MAX + 1];
    gethostname(host, sizeof(host));
    int homeDirEnd = strlen(getenv("HOME"));
    strcpy(currentDir, cwd + homeDirEnd);

    if (strcmp(cwd, getenv("HOME")) == 0) printf("\x1B[1;36m%s@%s\x1B[0m:~$ ", user, host);
    else if (strstr(cwd, getenv("HOME")) != NULL) printf("\x1B[1;36m%s@%s\x1B[0m:~%s$ ", user, host, currentDir);
    else printf("\x1B[1;36m%s@%s\x1B[0m:%s$ ", user, host, cwd);
    
    free(cwd);
    
}

/* ~~~ MAIN FUNCTION ~~~ */

int main(int argc, char **argv){ 

	struct sigaction sa; //TODO: config the sigaction struct parameters		
    
	char input[MAX_INPUT] = {0}; //unmodified user's input is stored here
	char *args[MAX_INPUT]; //User's input is stored here for later modifications
      
    	puts("You are now using \x1B[1;36mRADISH\x1B[0m made by \x1B[1;36mHFMaker\x1B[0m"); 
    

    	while (1){ //Main loop
        
            /* ~~~ INPUT HANDLING BLOCK ~~~ */

            rawModeON();
		    printPrompt();
		    fflush(stdout);
    
            char ch;
            int charIndex = 0;
 
            while (read(STDIN_FILENO, &ch, 1) == 1){

                int c = (unsigned char) ch;
                if (c == '\n') { write(STDOUT_FILENO, "\n", 1); rawModeOFF(); break; } //Enter
                else if (c == 4) break;
                else if (c == 3) { memset(input, 0, sizeof(input)); charIndex = 0; break; } //Ctrl + C
                else if (c == 127 || c == 8) { // Backspace
                    if (charIndex > 0) { 
                        charIndex--; 
                        write(STDOUT_FILENO, "\b \b", 3); 
                    }
                    continue;
                }

                else if (c == 0x1b){

                    char seq[2];
                    read(STDIN_FILENO, &seq[0], 1);
                    read(STDIN_FILENO, &seq[1], 1);

                    if (seq[0] == '['){

                        switch(seq[1]){

                            case 'A': break;
                            case 'B': break;
                            case 'C': break;
                            case 'D': break;

                        }
                    } 
                }

                else {

                    if (charIndex < MAX_INPUT - 1){

                        input[charIndex++] = c;
                        write(STDOUT_FILENO, &c, 1);

                    }
                }
            }

            input[charIndex] = '\0';
            if (strlen(input) == 0) continue;
		
		    for (int i = 0; pipeIndexes[i] != '\0'; i++) pipeIndexes[i] = 0;
		    for (int i = 0; redirIndexes[i] != '\0'; i++) redirIndexes[i] = 0;
		    numPipes = 0;
		    numRedirs = 0;
		    pipeIndex = 0;
		    redirIndex = 0;
            redirProc = 0;
            
            rawModeOFF();
		    parser(input, args);

            /* ~~~ DEBUGGING PRINTS ~~~ */

		    //for (int i = 0; redirIndexes[i] != NULL; i++) printf("Redirection number %d found on input index: %d\n", i, redirIndexes[i]);
            //for (int i = 0; args[i] != NULL; i++) printf("Argumen number %d: %s\n", i, args[i]); 


		    /* ~~~ BUILT-INT COMMANDS BLOCK ~~~ */
		     
		    if (strcmp(args[0], "cd") == 0){ 
			 if (!args[1] || strcmp(args[1], "~") == 0){
			    chdir(getenv("HOME")); //get the user's home directory
			 } else{
			     if (chdir(args[1]) != 0){
				perror("cd");
			    }
			 }
			continue;
			
		    }

		    
		    if (strcmp(args[0], "exit") == 0) {rawModeOFF(); exit(0);}

		    if (strcmp(args[0], "openshell") == 0){
			/* Open a new shell if "openshell" is the first argument
			 * This won't work if you don't have a desktop environment*/
			rawModeOFF();
			pid_t pid = fork();

			if (pid == 0)
			{
			    char *term = getenv("TERMINAL"); //get the user's default terminal

			    if (term){
				execlp(term, term, "-e", "./usr/bin/radish", NULL);
				execlp(term, term, "--", "./usr/bin/radish", NULL);
			    }

			    execlp("x-terminal-emulator", "x-terminal-emulator", "-e", "radish", NULL);
			    execlp("kitty", "kitty", "-e", "radish", NULL);
			    execlp("konsole", "konsole", "-e", "radish", NULL);
			    execlp("gnome-terminal", "gnome-terminal", "--", "radish", NULL);
			    execlp("xfce4-terminal", "xfce4-terminal", "-e", "radish", NULL);
			    execlp("alacritty", "alacritty", "-e", "radish", NULL);
			    execlp("mate-terminal", "mate-terminal", "-e", "radish", NULL);
			    execlp("xterm", "xterm", "-e", "radish", NULL);
			    perror("Compatible terminal not found");
			    rawModeOFF();
			    exit(1);
			    execvp(args[0], args);
			    perror("execvp");
			    rawModeOFF();
			    exit(1);
			}
		    

			wait(NULL);
			continue;
			
		    }

            /* ~~~ PIPELINE BLOCK ~~~ */
		 
		    if (flags & PIPELINE_FLAG){
			       
			    int procs = numPipes + 1; //Number of process
			    int fd[2 * numPipes];//All file descriptors for the pipes
			    int fdRedirs[2 * numRedirs];//All file descriptores for the redirections
			    int fdIndex = 0;
			    int start[procs];
			    start[0] = 0;

			    for (int i = 0; i < numPipes; i++) if (pipe(&fd[2 * i]) == -1){perror("pipe"); return 0;}   
			    for (int i = 0; pipeIndexes[i] != '\0'; i++) args[pipeIndexes[i]] = NULL;
			    			    
			    pid_t pids[procs];

			    for (int i = 0; i < procs; i++){
				rawModeOFF();
				pids[i] = fork();

				if (pids[i] == 0) {

				    if (i == 0) dup2(fd[1], STDOUT_FILENO);
				    else if (i == procs - 1) dup2(fd[2 * (numPipes - 1)], STDIN_FILENO);
				    else {dup2(fd[2 * (i - 1)], STDIN_FILENO); dup2(fd[2 * i + 1], STDOUT_FILENO);}

                    if (i == procs - 1){
                        for (int j = 0; j < numRedirs; j++){

                            int redirStatus = 1;

                            //for (int k = 0; k < redirIndexes[j]; k++) if (strcmp(args[k], "|") == 0) redirProc++;

                            if (strcmp(args[redirIndexes[j]], ">") == 0) redirFlags = O_WRONLY | O_CREAT | O_TRUNC; 
                            else if (strcmp(args[redirIndexes[j]], "<") == 0){redirFlags = O_RDONLY; redirStatus = 0;}
                            else if (strcmp(args[redirIndexes[j]], ">>") == 0) redirFlags = O_WRONLY | O_CREAT | O_APPEND;
                        
                            int fdRedir = open(args[redirIndexes[j] + 1], redirFlags, 0644);

                            if (fdRedir == -1){
                                perror("open"); 
                                return 0;
                            }

                            dup2(fdRedir, redirStatus);
                            close(fdRedir);

                            }
        
                    }

                    for (int i = 0; redirIndexes[i] != '\0'; i++){ args[redirIndexes[i]] = NULL; args[redirIndexes[i] + 1] = NULL;}


				    for (int i = 0; i < 2 * numPipes; i++) close(fd[i]);
                    for (int i = 0; i < 2 * numRedirs; i++) close(fdRedirs[i]);
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

            /* ~~~ REDIRECTIONS BLOCK ~~~ */

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
			rawModeOFF();
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

            /* ~~~ SIMPLE COMMANDS BLOCK ~~~ */
		    
		    rawModeOFF();
		    pid_t pid = fork();

		    if (pid == 0){
			execvp(args[0], args);
			printf("-radish: %s: command not found\n", args[0]);
			exit(1);
		    }
		    else wait(NULL);
		    
		    }
	
	rawModeOFF();
	return 0; 
}



